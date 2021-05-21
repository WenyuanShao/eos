#include <sl.h>
#include <sl_consts.h>
#include <sl_mod_policy.h>
#include <sl_plugins.h>
#include <bitmap.h>

#ifdef SL_MOD_DEBUG
#define debug printc
#else
#define debug(fmt,...)
#endif

#define DEADLINE_QUANTUM_LEN (500 * 2100)
#define LVL1_BITMAP_SZ       1
#define LVL2_BITMAP_SZ       (32)
#define DEADLINE_WINDOW_SZ   (LVL2_BITMAP_SZ * 32)
#define DEADLINE_MAX         ((DEADLINE_WINDOW_SZ - 1) * DEADLINE_QUANTUM_LEN)

struct sched_bitmaps {
	u64_t now_quantized;
	u32_t lvl1[LVL1_BITMAP_SZ];
	u32_t lvl2[LVL2_BITMAP_SZ];
	struct ps_list_head runqs[DEADLINE_WINDOW_SZ];
	struct ps_list_head low_prio_rq;
} __attribute__((aligned(64)));

struct sched_bitmaps runqueues[NUM_CPU];

static inline void
bm_print(unsigned long input)
{
	int b[32];
	int i = 0, count = 0;

	printc("0x%lx------->", input);
	for (i = 0; i < 32; i++) {
		b[i] = input % 2;
		input /= 2;
	}
	i = 32;
	while (i) {
		printc("%u", b[--i]);
		if (++count % 4 ==  0 && i){
			printc(" ");
			count = 0;
		}
	}
	printc("\n");
}

static inline u64_t
curr_offset(void)
{
	return ps_tsc() / DEADLINE_QUANTUM_LEN;
}

void
bitmap_edf_update_offsets(struct sched_bitmaps *rq)
{
	struct sl_thd_policy *node, *t = NULL;
	u64_t current_offset = curr_offset();
	u64_t now_offset;
	int i = 0;

	unsigned long long start, end;

	for (now_offset = rq->now_quantized; now_offset < current_offset; now_offset++) {
		assert(rq->now_quantized < current_offset);
		unsigned long now  = now_offset % DEADLINE_WINDOW_SZ;
		unsigned long next = (now_offset + 1) % DEADLINE_WINDOW_SZ;

		if (!ps_list_head_empty(&rq->runqs[now])) {

			ps_list_foreach_d(&rq->runqs[now], node) {
				assert(node->prio_idx == (int)now);
				node->prio_idx = (int)next;
				assert(sl_mod_thd_get(node)->state == SL_THD_RUNNABLE);
			}

			ps_list_head_append_all_d(&rq->runqs[next], &rq->runqs[now]);
			assert(!ps_list_head_empty(&rq->runqs[next]));
			assert(ps_list_head_empty(&rq->runqs[now]));
			bitmap_unset(rq->lvl2, now);
			if (rq->lvl2[now/LVL2_BITMAP_SZ] == 0) bitmap_unset(rq->lvl1, now/LVL2_BITMAP_SZ);
			bitmap_set(rq->lvl2, next);
			bitmap_set(rq->lvl1, next/LVL2_BITMAP_SZ);
		}
	}
	rq->now_quantized = current_offset;
}

u32_t rotr32(u32_t value, unsigned int rotation)
{
	return value >> rotation | value << (32 - rotation);
}

unsigned int
first_bit_lvl1(u32_t lvl1, unsigned int rotation)
{
	u32_t rotated;
	unsigned int ret;

	assert(rotation < 32);
	rotated = rotr32(lvl1, rotation);

	ret = __builtin_ctzl(rotated);
	if (ret < (LVL2_BITMAP_SZ - rotation))
		ret += rotation;
	else
		ret -= (LVL2_BITMAP_SZ - rotation);
	/*if (ret>=32) {
		printc("\trotat: %d----->", rotation);
		bm_print(rotated);
	}*/
	return ret;
}

static inline int
__find_first_bit(unsigned long bitmap)
{
	return __builtin_ctzl(bitmap);
}

static inline void
__check_list(struct sched_bitmaps* rq, int pos)
{
	if (ps_list_head_empty(&rq->runqs[pos])) {
		bitmap_unset(rq->lvl2, pos);
		if (rq->lvl2[pos/LVL2_BITMAP_SZ] == 0) bitmap_unset(rq->lvl1, pos/LVL2_BITMAP_SZ);
	}
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

void
sl_mod_execution(struct sl_thd_policy *t, cycles_t cycles)
{ }

struct sl_thd_policy *
sl_mod_schedule(void)
{
    struct sched_bitmaps *rq = &runqueues[cos_cpuid()];
	struct sl_thd_policy *thd;
	unsigned int rotation, tid;
	unsigned int first_lvl1, first_lvl2, pos;
	u32_t lvl2;
	struct ps_list_head *rq_head;
	int i= 0;

	bitmap_edf_update_offsets(rq);
	if (rq->lvl1[0] == (unsigned long)0) {
		if (ps_list_head_empty(&rq->low_prio_rq)) {
			if (cos_cpuid() == 4)
				printc("!");
			return NULL;
		} else {
			return ps_list_head_first_d(&rq->low_prio_rq, struct sl_thd_policy);
		}
	}

	rotation = rq->now_quantized % DEADLINE_WINDOW_SZ;
	first_lvl1 = first_bit_lvl1(rq->lvl1[0], rotation/LVL2_BITMAP_SZ);
	first_lvl2 = __builtin_ctzl(rq->lvl2[first_lvl1]);
	pos = (first_lvl1 * LVL2_BITMAP_SZ) + first_lvl2;
	assert(pos < 1024);
	rq_head = &rq->runqs[pos];

	if(!ps_list_head_empty(rq_head)) {
		thd = ps_list_head_first_d(rq_head, struct sl_thd_policy);
	} else {
		__check_list(rq, pos);
		return NULL;
	}
	assert(thd->prio_idx == pos);
	return thd;
}

void
sl_mod_block(struct sl_thd_policy *t)
{
	struct sched_bitmaps *rq = &runqueues[cos_cpuid()];
	int pos = t->prio_idx;

	bitmap_edf_update_offsets(rq);
	ps_list_rem_d(t);

	if (pos != -1) {
		__check_list(rq, pos);
		t->prio_idx = -1;
	}
	sl_mod_thd_get(t)->state = SL_THD_BLOCKED;
	bitmap_edf_update_offsets(rq);
	assert(t->prio_idx == -1);
}

void
sl_mod_wakeup(struct sl_thd_policy *t)
{
	struct sched_bitmaps *rq = &runqueues[cos_cpuid()];
	unsigned long long dl_quantized = (t->deadline / DEADLINE_QUANTUM_LEN);
	unsigned long long now_quantized = 0;
	int pos = t->prio_idx;

	bitmap_edf_update_offsets(rq);
	now_quantized = rq->now_quantized;
	assert(t->deadline);
	assert(sl_mod_thd_get(t)->wakeup_cnt > 0);

	ps_list_rem_d(t);
	if (dl_quantized < now_quantized) {
	/* if already passed its deadline when wakeup, let it exectue once */
		dl_quantized = now_quantized;
	}
	assert(dl_quantized < (now_quantized + DEADLINE_WINDOW_SZ));

	/* Wakeup a thread which is already active after modifying its deadline */
	if (pos >= 0) {
		__check_list(rq, pos);
	}
	dl_quantized %= DEADLINE_WINDOW_SZ;
	assert(t->prio_idx == -1);
	t->prio_idx = dl_quantized;

	ps_list_head_append_d(&rq->runqs[dl_quantized], t);
	assert(!ps_list_head_empty(&rq->runqs[dl_quantized]));
	bitmap_set(rq->lvl2, dl_quantized);
	bitmap_set(rq->lvl1, dl_quantized/LVL2_BITMAP_SZ);
}

void
sl_mod_yield(struct sl_thd_policy *t, struct sl_thd_policy *yield_to)
{
	struct sched_bitmaps *rq = &runqueues[cos_cpuid()];
	bitmap_edf_update_offsets(rq);
	int pos = t->prio_idx;

	ps_list_rem_d(t);
	if (t->deadline < ps_tsc() || pos == -1) {
		t->prio_idx = -1;
		__check_list(rq, pos);
		ps_list_head_append_d(&rq->low_prio_rq, t);
		return;
	}
	assert(t->prio_idx >= 0);
	assert(t->prio_idx < DEADLINE_WINDOW_SZ);
	ps_list_head_append_d(&rq->runqs[pos], t);
	bitmap_set(rq->lvl2, pos);
}

void
sl_mod_thd_create(struct sl_thd_policy *t)
{
	struct sched_bitmaps *rq = &runqueues[cos_cpuid()];
	unsigned long long tmp = (unsigned long long)500 * (unsigned long long)1000 * (unsigned long long)2100;

	t->period      = 0;
    t->period_usec = 0;
   	t->priority = t->deadline = ps_tsc() + tmp;
	t->prio_idx    = -1;
	t->deadline    = 0;
    
	ps_list_init_d(t);
}

void
sl_mod_thd_delete(struct sl_thd_policy *t)
{
	struct sched_bitmaps *rq = &runqueues[cos_cpuid()];

	assert(0);

	ps_list_rem_d(t);
	__check_list(rq, t->prio_idx);
}

void
sl_mod_thd_param_set(struct sl_thd_policy *t, sched_param_type_t type, unsigned long long v)
{
	struct sched_bitmaps *rq = &runqueues[cos_cpuid()];
	int i = 0, dl_quantized;
	unsigned long long pos;
	unsigned long long curr;

	bitmap_edf_update_offsets(rq);
	switch (type) {
		case SCHEDP_PRIO:
		{
			assert(cos_cpuid() == 0);
			t->priority = v;
			sl_thd_setprio(sl_mod_thd_get(t), t->priority);
			ps_list_rem_d(t);
			dl_quantized = DEADLINE_WINDOW_SZ - 1 + rq->now_quantized;
			t->deadline = dl_quantized * DEADLINE_QUANTUM_LEN;
			ps_list_rem_d(t);
			assert(t->prio_idx == -1);
			__check_list(rq, t->prio_idx);

			/* FIXME: the normal path should be modified as always call thd wakeup after set param. */
			ps_list_head_append_d(&rq->runqs[DEADLINE_WINDOW_SZ-1], t);
			t->prio_idx = DEADLINE_WINDOW_SZ-1;
			bitmap_set(rq->lvl2, t->prio_idx);
			bitmap_set(rq->lvl1, t->prio_idx/LVL2_BITMAP_SZ);
			assert(cos_cpuid() == 0);
			break;
		}
		case SCHEDP_PRIO_BLOCK:
		{
			/* should not be used */
			assert(0);
			/*t->priority = v;
			sl_thd_setprio(sl_mod_thd_get(t), t->priority);
			sl_mod_thd_get(t)->state = SL_THD_BLOCKED;
			dl_quantized = DEADLINE_WINDOW_SZ - 1 + rq->now_quantized;
			t->deadline = dl_quantized * DEADLINE_QUANTUM_LEN;
			sl_mod_thd_get(t)->state = SL_THD_BLOCKED;
			t->prio_idx = -1;
			break;*/
		}
		case SCHEDP_WINDOW:
		{
			assert(0);
			break;
		}
		case SCHEDP_DEADLINE:
		{
			sl_mod_thd_get(t)->state = SL_THD_BLOCKED;
			t->deadline = v;
			curr = ps_tsc();
			unsigned long long tmp = (unsigned long long)500 * (unsigned long long)1000 * (unsigned long long)2100;
			if (v > curr+tmp) {
				printc("deadline: %llu, MAX: %llu\n", v, ~(cycles_t)0);
			}
			assert(v <= curr + tmp);
			t->prio_idx = -1;

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
	struct sched_bitmaps *rq = &runqueues[cos_cpuid()];
	int i;

	memset(rq, 0, sizeof(struct sched_bitmaps));
	rq->now_quantized = curr_offset();
	ps_list_head_init(&rq->low_prio_rq);
	for (i = 0; i < DEADLINE_WINDOW_SZ; i++) {
		ps_list_head_init(&rq->runqs[i]);
	}
}
