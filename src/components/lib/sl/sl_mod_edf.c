#include <sl.h>
#include <sl_consts.h>
#include <sl_mod_policy.h>
#include <sl_plugins.h>
#include <heap.h>

#ifdef SL_MOD_DEBUG
#define debug printc
#else
#define debug(fmt,...)
#endif

#define SL_EDF_MAX_THDS     MAX_NUM_THREADS
#define SL_EDF_MAX_DEADLINE ~(cycles_t)0
#define SL_EDF_DL_LOW       TCAP_PRIO_MIN

struct edf_heap {
    struct heap h;
    void       *data[SL_EDF_MAX_THDS];
    char        p; /* pad */
};

static struct edf_heap edf_heap[NUM_CPU];
static struct heap *hs[NUM_CPU];
static unsigned long long sl_per_core[NUM_CPU];
static unsigned long long global_start[NUM_CPU];
//static struct heap *hs = (struct heap *)&edf_heap;

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

int print_ctl = 0;
sl_ulti_print(int cpu)
{
	unsigned long long global_end;

	global_end = ps_tsc();
	if ((global_end-global_start[cpu]) >= (unsigned long long)10000000 * (unsigned long long) 2700) {
		if (cpu == 4 && print_ctl == 0) {
			printc("total: %llu, sl total: %llu\n", (global_end-global_start[cpu]), sl_per_core[cpu]);
			print_ctl = 1;
		}
	}
}

void
sl_mod_execution(struct sl_thd_policy *t, cycles_t cycles)
{ }

struct sl_thd_policy *
sl_mod_schedule(void)
{
	struct sl_thd_policy *t;
	unsigned long long start, end;

	start = ps_tsc();
    int cpu = cos_cpuid();

    t = heap_peek(hs[cpu]);
	end = ps_tsc();
	sl_per_core[cpu] += (end-start);
	
	return t;
}

void
sl_mod_block(struct sl_thd_policy *t)
{
	unsigned long long start, end;
	start = ps_tsc();
    int cpu = cos_cpuid();

	assert(t->prio_idx >= 0);
	
	heap_remove(hs[cpu], t->prio_idx);
	sl_mod_thd_get(t)->state = SL_THD_BLOCKED;
	debug("block= remove idx: %d, deadline: %llu\n", t->prio_idx, t->deadline);
	t->prio_idx  = -1;
	
	end = ps_tsc();
	sl_per_core[cpu] += (end-start);
}

void
sl_mod_wakeup(struct sl_thd_policy *t)
{
	unsigned long long start, end;
	start = ps_tsc();
	int cpu = cos_cpuid();
	assert(t->deadline != SL_EDF_MAX_DEADLINE);
	if(t->prio_idx < 0) {
		heap_add(hs[cpu], t);
	} else {
		assert(t->prio_idx > 0);
		heap_adjust(hs[cpu], t->prio_idx);
	}
	end = ps_tsc();
	sl_per_core[cpu] += (end-start);
}

void
sl_mod_yield(struct sl_thd_policy *t, struct sl_thd_policy *yield_to)
{
	//assert(0);
}

void
sl_mod_thd_create(struct sl_thd_policy *t)
{
    int cpu = cos_cpuid();

	t->period      = 0;
    t->period_usec = 0;
    t->priority    = t->deadline = SL_EDF_MAX_DEADLINE;
    t->prio_idx    = -1;
    
    assert((heap_size(hs[cpu])+1) < SL_EDF_MAX_THDS);
    debug("create= add idx: %d, deadline: %llu\n", t->prio_idx, t->deadline);
}

void
sl_mod_thd_delete(struct sl_thd_policy *t)
{ heap_remove(hs[cos_cpuid()], t->prio_idx); }

void
sl_mod_thd_param_set(struct sl_thd_policy *t, sched_param_type_t type, unsigned long long v)
{
    cycles_t now;
	unsigned long long start, end;
	start = ps_tsc();
	int cpu = cos_cpuid();

	rdtscll(now);
	switch (type) {
		case SCHEDP_PRIO:
		{
			t->priority = v;
			sl_thd_setprio(sl_mod_thd_get(t), t->priority);
			t->deadline = SL_EDF_MAX_DEADLINE;
			assert(t->prio_idx < 0);
			heap_add(hs[cpu], t);
			break;
		}
		case SCHEDP_PRIO_BLOCK:
		{
			t->priority = v;
			sl_thd_setprio(sl_mod_thd_get(t), t->priority);
			sl_mod_thd_get(t)->state = SL_THD_BLOCKED;
			t->deadline = SL_EDF_MAX_DEADLINE;
			assert(t->prio_idx < 0);
			break;
		}
		case SCHEDP_WINDOW:
		{
			/* t->period_usec = v;
			 * t->period = sl_usec2cyc(t->period_usec);

			 * first deadline.
			 * t->deadline = now + t->period;
			 *
			 * TODO: 1. tcap_prio_t=48bit! mapping 64bit value to 48bit value.
			 *          (or, can we make cos_switch/cos_tcap_delegate support prio=64bits?).
			 *       2. wraparound (64bit or 48bit) logic for deadline-based-heap!
			 *
			 * t->priority = t->deadline;
			 * assert(t->priority <= SL_EDF_DL_LOW);
			 * assert(t->prio_idx > 0);
			 * heap_adjust(hs, t->prio_idx);
			 * debug("param_set= adjust idx: %d, deadline: %llu\n", t->prio_idx, t->deadline);
			 */
			assert(0);
			break;
		}
		case SCHEDP_DEADLINE:
		{
			t->deadline = v;
			t->priority = t->deadline;
			debug("param_set= adjust idx: %d, deadline: %llu\n", t->prio_idx, t->deadline);
			break;
		}
		default:
		{
			assert(0);
			break;
		}
	}
	end = ps_tsc();
	sl_per_core[cpu] += (end-start);
}

static int
__compare_min(void *a, void *b)
{ return ((struct sl_thd_policy *)a)->deadline <= ((struct sl_thd_policy *)b)->deadline; }

static void
__update_idx(void *e, int pos)
{ ((struct sl_thd_policy *)e)->prio_idx = pos; }

void
sl_mod_init(void)
{
	int cpu = cos_cpuid();

	hs[cpu] = (struct heap *)(&edf_heap[cpu]);
	heap_init(hs[cpu], SL_EDF_MAX_THDS, __compare_min, __update_idx);
	memset(sl_per_core, 0, sizeof(unsigned long long) * NUM_CPU);
	global_start[cpu] = ps_tsc();
	print_ctl = 0;
}
