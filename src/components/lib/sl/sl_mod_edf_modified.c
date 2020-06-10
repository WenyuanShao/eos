#include <sl.h>
#include <sl_consts.h>
#include <sl_mod_policy.h>
#include <sl_plugins.h>

#ifdef SL_MOD_DEBUG
#define debug printc
#else
#define debug(fmt,...)
#endif

#define SL_EDF_MAX_DEADLINE ~(cycles_t)0
#define SL_WINDOW_SZ        1024
#define SL_WORD_SZ          32
#define INTERVAL_SZ         (2700 * 1000)

struct base_info {
	unsigned long long base;
	int                head;
};

volatile unsigned long bm_lv2[NUM_CPU][SL_WORD_SZ];
volatile unsigned long bm_lv1[NUM_CPU];
struct ps_list_head threads[NUM_CPU][SL_WINDOW_SZ] CACHE_ALIGNED;
volatile struct base_info base_info[NUM_CPU];

static inline int
__find_first_bit(unsigned long bitmap)
{
	return __builtin_ctzl(bitmap);
}

static inline void
__set_bit(unsigned long *bitmap, int pos)
{
	assert(pos < SL_WORD_SZ && pos >= 0);
	*bitmap |= (0x1 << pos);
	return;
}

static inline int
__verify_bit(unsigned long *bitmap, int pos)
{
	unsigned long ret;
	assert(pos < SL_WORD_SZ && pos >= 0);
	ret = *bitmap & (0x1 << pos);
	if (ret == (unsigned long)0U)
		return 0;
	else
		return 1;
}

static inline void
__clear_bit(unsigned long *bitmap, int pos)
{
	assert(pos < SL_WORD_SZ && pos >= 0);
	*bitmap &= ~(0x1 << pos);
	return;
}

static inline void
__check_clear(int l_pos)
{
	int cpu = cos_cpuid();
	int bm_pos = 0;

	assert(l_pos >= 0 & l_pos < 1024);

	if (l_pos >= base_info[cpu].head)
		bm_pos = l_pos - base_info[cpu].head;
	else
		bm_pos = l_pos - base_info[cpu].head + SL_WINDOW_SZ;
	if (ps_list_head_empty(&threads[cpu][l_pos])) {
		__clear_bit(&bm_lv2[cpu][bm_pos/SL_WORD_SZ], bm_pos%SL_WORD_SZ);
		if (!bm_lv2[cpu][pos/SL_WORD_SZ])
			__clear_bit(&bm_lv1[cpu], bm_pos/SL_WORD_SZ);
	}
	return;
}

void
sl_mod_execution(struct sl_thd_policy *t, cycles_t cycles)
{ }

unsigned long long previous[NUM_CPU];
volatile int pcnt = 1;

struct sl_thd_policy *
sl_mod_schedule(void)
{
    int lv1_idx, lv2_idx, idx, cpu = cos_cpuid();
    struct sl_thd_policy *thd = NULL;
	struct sl_thd_policy *node, *t = NULL;
	unsigned long long now;
	int head, legacy = 0;
	int i = 0, cnt = 0;

	now = ps_tsc();

	/* If INTERVAL_SZ greater than a timer-tick, this while loop can be replaced by an if */
	while (now > (base_info[cpu].base + INTERVAL_SZ)) {
		head = base_info[cpu].head;
		if (!ps_list_head_empty(&threads[cpu][head])) {
			ps_list_foreach_del_d(&threads[cpu][head], node, t) {
				ps_list_rem_d(node);
				ps_list_head_add_d(&threads[cpu][head+1], node);
				
				assert(node->prio_idx == head);
				node->prio_idx = (head+1)&(SL_WINDOW_SZ-1);
				assert(!ps_list_head_empty(&threads[cpu][head+1]));
			}
			legacy = 1;
		}
		base_info[cpu].head ++;
		base_info[cpu].head &= (SL_WINDOW_SZ - 1);
		base_info[cpu].base += INTERVAL_SZ;
		cnt++;
	}

	/* Reconstruct bitmap after moving the head of the list array */
	if (likely(cnt)) {
		assert(cnt < 32);
		bm_lv1[cpu] = (unsigned long)0U;

		for (i = 0; i < SL_WORD_SZ-1; i++) {
			bm_lv2[cpu][i] = (bm_lv2[cpu][i+1] << (SL_WORD_SZ - cnt)) | (bm_lv2[cpu][i] >> cnt);
			if (bm_lv2[cpu][i])	__set_bit(&bm_lv1[cpu], i);
		}
		bm_lv2[cpu][SL_WORD_SZ-1] = bm_lv2[cpu][SL_WORD_SZ-1] >> cnt;
		bm_lv2[cpu][0] |= (unsigned long)legacy;
		if(ps_list_head_empty(&threads[cpu][base_info[cpu].head]) != legacy) {
			printc("ERROR: %d\n", legacy);
		}

		if (bm_lv2[cpu][SL_WORD_SZ-1])	__set_bit(&bm_lv1[cpu], SL_WORD_SZ-1);
		bm_ver[cpu] = bm_lv1[cpu];
	}

	if (bm_lv1[cpu] == 0) {
		for (i = 0; i < SL_WORD_SZ; i++) {
			assert(!bm_lv2[cpu][i]);
		}
		return NULL;
	}

	lv1_idx = __find_first_bit(bm_lv1[cpu]);
	lv2_idx = __find_first_bit(bm_lv2[cpu][lv1_idx]);
	idx = lv1_idx*SL_WORD_SZ + lv2_idx + base_info[cpu].head;
	idx &= (SL_WINDOW_SZ-1);
	
	if (!ps_list_head_empty(&threads[cpu][idx])) {
		thd = ps_list_head_first_d(&threads[cpu][idx], struct sl_thd_policy);
		assert(thd);
		assert(thd->prio_idx == idx);
		return thd;
	}
	assert(0);

	return NULL;
}

void
sl_mod_block(struct sl_thd_policy *t)
{
	int cpu = cos_cpuid();
	int pos;
	
	pos = t->prio_idx;
	assert((pos < SL_WINDOW_SZ) && (pos >= 0));

	ps_list_rem_d(t);

	__check_clear(pos);
	sl_mod_thd_get(t)->state = SL_THD_BLOCKED;
}

void
sl_mod_wakeup(struct sl_thd_policy *t)
{
	int cpu = cos_cpuid();
	int l_pos, bm_pos;
	unsigned long long now;

	assert(t->deadline);
	assert(t->deadline < SL_EDF_MAX_DEADLINE);
	
	if (t->deadline < base_info[cpu].base) {
		bm_pos = 0;
	} else {
		bm_pos = (int)(t->deadline - base_info[cpu].base)/INTERVAL_SZ;
	}
	assert((bm_pos < SL_WINDOW_SZ) && (bm_pos >= 0));
	l_pos = (bm_pos + base_info[cpu].head) & (SL_WINDOW_SZ - 1);
	t->prio_idx = l_pos;

	/* Wakeup a thread which is already active after modifying its deadline */
	ps_list_rem_d(t);
	__check_clear(l_pos);

	ps_list_head_add_d(&threads[cpu][l_pos], t);
	__set_bit(&bm_lv2[cpu][bm_pos/SL_WORD_SZ], bm_pos%SL_WORD_SZ);
	__set_bit(&bm_lv1[cpu], bm_pos/SL_WORD_SZ);

}

void
sl_mod_yield(struct sl_thd_policy *t, struct sl_thd_policy *yield_to)
{
	assert(t->prio_idx >=0 && t->prio_idx < SL_WINDOW_SZ);
	ps_list_rem_d(t);
	ps_list_head_append_d(&threads[cos_cpuid()][t->prio_idx], t);
}

void
sl_mod_thd_create(struct sl_thd_policy *t)
{
    t->period      = 0;
    t->period_usec = 0;
    t->priority    = t->deadline = SL_EDF_MAX_DEADLINE;
	t->prio_idx    = -1;
    
	ps_list_init_d(t);
}

void
sl_mod_thd_delete(struct sl_thd_policy *t)
{
	int cpu = cos_cpuid();

	ps_list_rem_d(t);

	__check_clear(t->prio_idx);
	t->prio_idx = -1;
}

void
sl_mod_thd_param_set(struct sl_thd_policy *t, sched_param_type_t type, unsigned long long v)
{
	int cpu = cos_cpuid();
	int i = 0, last;
	
	unsigned long long pos;

	switch (type) {
		case SCHEDP_PRIO:
		{
			t->deadline = SL_EDF_MAX_DEADLINE;
			last = (SL_WINDOW_SZ - 1 + base_info[cpu].head) & (SL_WINDOW_SZ - 1);
			ps_list_rem_d(t);
			/*
			 * FIXME: the normal path should be modified as always call thd wakeup after set param. 
			 * Here we wake it up after setting the parameter.
			 */
			ps_list_head_append_d(&threads[cpu][last], t);
			__set_bit(&bm_lv2[cpu][last/SL_WORD_SZ], last%SL_WORD_SZ);
			__set_bit(&bm_lv1[cpu], last/SL_WORD_SZ);
			break;
		}
		case SCHEDP_PRIO_BLOCK:
		{
			sl_mod_thd_get(t)->state = SL_THD_BLOCKED;
			t->deadline = SL_EDF_MAX_DEADLINE;
			break;
		}
		case SCHEDP_WINDOW:
		{
			assert(0);
			break;
		}
		case SCHEDP_DEADLINE:
		{
			t->deadline = v;
			if(sl_mod_thd_get(t)->state != SL_THD_BLOCKED) {
				pos = (t->deadline - base_info[cpu].base)/INTERVAL_SZ + base_info[cpu].head;
				assert((pos >= 0) & (pos < SL_EDF_MAX_DEADLINE));
				pos &= (SL_WINDOW_SZ - 1);
			}
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

	memset(threads[cpu], 0, sizeof(struct ps_list_head) * SL_WINDOW_SZ);
	memset(&base_info[cpu], 0, sizeof(struct base_info));
	memset(bm_lv2[cpu], 0, sizeof(unsigned long) * SL_WORD_SZ);
	memset(&bm_lv1[cpu], 0, sizeof(unsigned long));
	
	base_info[cpu].base = ps_tsc();

	for (i = 0; i < SL_WINDOW_SZ; i++) {
		ps_list_head_init(&threads[cpu][i]);
	}
}
