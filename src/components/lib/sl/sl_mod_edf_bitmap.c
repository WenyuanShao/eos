#include <sl.h>
#include <sl_consts.h>
#include <sl_mod_policy.h>
#include <sl_plugins.h>

#ifdef SL_MOD_DEBUG
#define debug printc
#else
#define debug(fmt,...)
#endif

#define SL_EDF_MAX_THDS     MAX_NUM_THREADS
#define SL_EDF_MAX_DEADLINE ~(cycles_t)0
//#define SL_EDF_SIZE         SL_EDF_MAX_DEADLINE + 1
#define SL_EDF_DL_LOW       TCAP_PRIO_MIN
#define SL_WINDOW_SZ        95
#define INTERVAL_SZ         2700 * 20

struct base_info {
	unsigned long long base;
	int                enqueued;
	int                to_sched;
};

static unsigned long bitmap[NUM_CPU][3];
struct ps_list_head threads[NUM_CPU][SL_WINDOW_SZ+1] CACHE_ALIGNED;
struct ps_list_head to_sched[NUM_CPU] CACHE_ALIGNED;
static struct base_info base_info[NUM_CPU];

static inline int
__find_first_bit(unsigned long *bitmap)
{
	if (bitmap[0])
		return __builtin_ctzl(bitmap[0]);
	if (bitmap[1])
		return __builtin_ctzl(bitmap[1] + 32);
	return __builtin_ctzl(bitmap[2] + 32);
}

static inline void
__set_bit(unsigned long *bitmap, int pos)
{
	assert(pos <= SL_WINDOW_SZ);
	if (pos < 32) {
		bitmap[0] |= (0x1 << pos);
		return;
	}
	if (pos < 64) {
		bitmap[1] |= (0x1 << (pos - 32));
		return;
	}
	bitmap[2] |= (0x1 << (pos - 64));
	return;
}

static inline void
__clear_bit(unsigned long *bitmap, int pos)
{
	assert(pos <= SL_WINDOW_SZ);
	if (pos < 32) {
		bitmap[0] &= ~(0x1 << pos);
		return;
	}
	if (pos < 64) {
		bitmap[1] &= ~(0x1 << (pos - 32));
		return;
	}
	bitmap[2] &= ~(0x1 << (pos - 64));
	return;
}

thdid_t
sl_mod_gettid(struct sl_thd_policy *t)
{
	struct sl_thd *thd;
	thdid_t        tid;
	
	assert(t);
	thd = sl_mod_thd_get(t);
	assert(thd);
	tid = sl_thd_thdid(thd);
	assert(tid);

	return tid;
}

int
sl_mod_load(int cpu)
{
	int ret = 0;
	unsigned long long pos;
	unsigned long long now;
	struct sl_thd_policy *t;

	//if (ps_list_head_empty(&to_sched[cpu])) return 0;
	assert(base_info[cpu].enqueued == 0);
	if (base_info[cpu].to_sched == 0)	return 0;
	rdtscll(now);
	//printc("now: %lld\n", now);
	base_info[cpu].base = now;

	ps_list_foreach_d(&to_sched[cpu], t) {
		assert(t->deadline > base_info[cpu].base);
		pos = (t->deadline - base_info[cpu].base)/INTERVAL_SZ;
		if (pos < SL_WINDOW_SZ) {
			ps_list_rem_d(t);
			base_info[cpu].to_sched--;
			ps_list_head_add_d(&threads[cpu][pos], t);
			base_info[cpu].enqueued++;

			__set_bit(bitmap[cpu], (int)pos);
			ret ++;
			printc("\tload success! pos: %d: base: %llu, enqueued: %d\n", (int)pos, base_info[cpu].base, base_info[cpu].enqueued);
		}
		printc("\tfailed! pos: %d, base: %d, enqueued: %d, to_sched: %d\n", (int)pos, base_info[cpu].base, base_info[cpu].enqueued, base_info[cpu].to_sched);
	}
	/*if (ret == 0) {
		pos = (t->deadline - base_info[cpu].base)/INTERVAL_SZ;
		//printc("error: %d\n", (int)pos);
		assert(0);
	}*/
	//assert(ret > 0);
	return ret;
}

void
sl_mod_execution(struct sl_thd_policy *t, cycles_t cycles)
{ }

unsigned long long previous[NUM_CPU];
int print_cnt;

struct sl_thd_policy *
sl_mod_schedule(void)
{
    int first, cpu = cos_cpuid(), ret;
    struct sl_thd_policy *t;
	unsigned long long start, end, now;
	int tid;
	start = ps_tsc();
	if (cpu == 4) {
		rdtscll(now);
		//printc("\tgap: %lluus\n", (now-previous[cpu])/2700);
		rdtscll(previous[cpu]);
		print_cnt++;
	}
	if ((bitmap[cpu][0]|bitmap[cpu][1]|bitmap[cpu][2]) == 0) {
		assert(bitmap[cpu][0] == 0);
		assert(bitmap[cpu][1] == 0);
		assert(bitmap[cpu][2] == 0);
		if (base_info[cpu].to_sched == 0) {
			return NULL;
		}
		ret = sl_mod_load(cpu);
		if (!ret) return NULL;
	}
	first = __find_first_bit(bitmap[cpu]);
	if (!ps_list_head_empty(&threads[cpu][first])) {
		t = ps_list_head_first_d(&threads[cpu][first], struct sl_thd_policy);
		tid = sl_mod_gettid(t);
		end = ps_tsc();
		//printc("schedlue latency: %llu\n", end-start);
		//printc("schedule: %d\n", tid);
		return t;
	}
	//if (cpu == 4) {
	//}

	return NULL;
}

void
sl_mod_block(struct sl_thd_policy *t)
{
	int cpu = cos_cpuid();
	unsigned long long pos;
	
	ps_list_rem_d(t);
	pos = (t->deadline - base_info[cpu].base)/INTERVAL_SZ;
	assert(pos >= 0);
	assert(pos < SL_WINDOW_SZ);

	if (ps_list_head_empty(&threads[cpu][(int)pos])) {
		__clear_bit(bitmap[cpu], (int)pos);
		base_info[cpu].enqueued--;
		//if (base_info[cpu].enqueued == 0)	base_info[cpu].base = 0;
	}
	sl_mod_thd_get(t)->state = SL_THD_BLOCKED;
	//printc("block; enqueued: %d, to_sched: %d\n", base_info[cpu].enqueued, base_info[cpu].to_sched);
}

void
sl_mod_wakeup(struct sl_thd_policy *t)
{
	int cpu = cos_cpuid();
	unsigned long long pos, now;

	ps_list_rem_d(t);
	if (base_info[cpu].base == 0) {
		assert(base_info[cpu].enqueued == 0);
		assert(base_info[cpu].to_sched == 0);
		base_info[cpu].base = t->deadline;
	}
	rdtscll(now);
	pos = (t->deadline - now)/INTERVAL_SZ;
	assert(pos >= 0);
	if (pos < SL_WINDOW_SZ) {
		ps_list_head_add_d(&threads[cpu][pos], t);
		base_info[cpu].enqueued++;
		__set_bit(bitmap[cpu], pos);
	} else {
		ps_list_head_add_d(&to_sched[cpu], t);
		base_info[cpu].to_sched++;
		printc("wakeup and add task into to_sched list: %d\n", pos);
	}

}

void
sl_mod_yield(struct sl_thd_policy *t, struct sl_thd_policy *yield_to)
{
	//printc("yield\n");
	//assert(0);
	//ps_list_rem_d(t);
	//ps_list_head_append_d(&threads[cos_cpuid()][t->deadline], t);
}

void
sl_mod_thd_create(struct sl_thd_policy *t)
{
    t->period      = 0;
    t->period_usec = 0;
    t->priority    = t->deadline = SL_EDF_MAX_DEADLINE;
    
	ps_list_init_d(t);
}

void
sl_mod_thd_delete(struct sl_thd_policy *t)
{
	int cpu = cos_cpuid();
	ps_list_rem_d(t);

	if (ps_list_head_empty(&threads[cpu][t->deadline])) {
		__clear_bit(bitmap[cpu], t->deadline);
		base_info[cpu].enqueued --;
	}
}

void
sl_mod_thd_param_set(struct sl_thd_policy *t, sched_param_type_t type, unsigned long long v)
{
    cycles_t now;
	int cpu = cos_cpuid();
	rdtscll(now);
	switch (type) {
		case SCHEDP_PRIO:
		{
			t->priority = v;
			sl_thd_setprio(sl_mod_thd_get(t), t->priority);
			t->deadline = SL_EDF_MAX_DEADLINE;
			//ps_list_rem_d(t);
			//ps_list_head_append_d(&threads[cpu][t->deadline], t);
			//__set_bit(bitmap[cpu], t->deadline);
			sl_mod_wakeup(t);
			break;
		}
		case SCHEDP_PRIO_BLOCK:
		{
			t->priority = v;
			sl_thd_setprio(sl_mod_thd_get(t), t->priority);
			sl_mod_thd_get(t)->state = SL_THD_BLOCKED;
			t->deadline = SL_EDF_MAX_DEADLINE;
			break;
		}
		case SCHEDP_WINDOW:
		{
			//t->period_usec = v;
			//t->period = sl_usec2cyc(t->period_usec);

			/* first deadline. */
			//t->deadline = now + t->period;
			/*
			 * TODO: 1. tcap_prio_t=48bit! mapping 64bit value to 48bit value.
			 *          (or, can we make cos_switch/cos_tcap_delegate support prio=64bits?).
			 *       2. wraparound (64bit or 48bit) logic for deadline-based-heap!
			 */
			//t->priority = t->deadline;
			//assert(t->priority <= SL_EDF_DL_LOW);
			//assert(t->prio_idx > 0);
			//heap_adjust(hs, t->prio_idx);
			//debug("param_set= adjust idx: %d, deadline: %llu\n", t->prio_idx, t->deadline);
			assert(0);
			break;
		}
		case SCHEDP_DEADLINE:
		{
			//t->deadline = v + now;
			t->deadline = v;
			t->priority = t->deadline;
			//assert(t->priority <= SL_EDF_DL_LOW);
			/*if (t->prio_idx > 0) {
				heap_adjust(hs, t->prio_idx);
			} else {
				heap_add(hs, t);
			}*/
			//struct sl_thd_policy *test = heap_peek(hs);
			debug("param_set= adjust idx: %d, deadline: %llu\n", t->prio_idx, t->deadline);
			break;
		}
		default:
		{
			assert(0);
			break;
		}
	}
}

void
sl_mod_init(void)
{
	int i, cpu = cos_cpuid();

	memset(threads[cpu], 0, sizeof(struct ps_list_head) * (SL_WINDOW_SZ+1));
	memset(&to_sched[cpu], 0, sizeof(struct ps_list_head));
	memset(&base_info[cpu], 0, sizeof(struct base_info));
	memset(bitmap[cpu], 0, sizeof(unsigned long) * 3);
	for (i = 0; i <= SL_WINDOW_SZ; i++) {
		ps_list_head_init(&threads[cpu][i]);
	}
	ps_list_head_init(&to_sched[cpu]);
	print_cnt = 0;
}
