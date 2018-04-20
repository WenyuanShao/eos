#include <sched.h>
#include <cos_thd_init.h>

int sched_thd_block_timeout_cserialized(u32_t *elapsed_hi, u32_t *elapsed_lo, thdid_t deptid, u32_t abs_hi, u32_t abs_lo);
thdid_t sched_thd_create_cserialized(thdclosure_index_t idx);
thdid_t sched_aep_create_cserialized(arcvcap_t *rcv, int *unused, thdclosure_index_t idx, int owntc, cos_channelkey_t key);

cycles_t
sched_thd_block_timeout(thdid_t deptid, cycles_t abs_timeout)
{
	u32_t elapsed_hi = 0, elapsed_lo = 0;
	cycles_t elapsed_cycles = 0;
	int ret = 0;

	ret = sched_thd_block_timeout_cserialized(&elapsed_hi, &elapsed_lo, deptid,
					     (u32_t)(abs_timeout >> 32), (u32_t)((abs_timeout << 32) >> 32));
	if (!ret) elapsed_cycles = ((cycles_t)elapsed_hi << 32) | (cycles_t)elapsed_lo;

	return elapsed_cycles;
}

thdid_t
sched_thd_create(cos_thd_fn_t fn, void *data)
{
	thdclosure_index_t idx = cos_thd_init_alloc(fn, data);

	if (idx < 1) return 0;

	return sched_thd_create_cserialized(idx);
}

thdid_t
sched_aep_create(struct cos_aep_info *aep, cos_aepthd_fn_t fn, void *data, int owntc, cos_channelkey_t key)
{
	thdclosure_index_t idx = cos_thd_init_alloc(cos_aepthd_fn, (void *)aep);
	arcvcap_t rcv;
	int ret;
	int unused;

	if (idx < 1) return 0;

	memset(aep, 0, sizeof(struct cos_aep_info));
	ret = sched_aep_create_cserialized(&rcv, &unused, idx, owntc, key);
	if (!ret) return 0;

	aep->fn   = fn;
	aep->data = data;
	aep->rcv  = rcv;
	aep->tid  = ret;

	return ret;
}
