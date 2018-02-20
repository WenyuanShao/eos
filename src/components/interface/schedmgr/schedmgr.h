#ifndef SCHEDMGR_H
#define SCHEDMGR_H

#include <cos_kernel_api.h>
#include <cos_defkernel_api.h>
#include <sl.h>
#include <res_spec.h>

int schedmgr_thd_wakeup(spdid_t c, thdid_t t);
int schedmgr_thd_block(spdid_t c, thdid_t dep_t);
int schedmgr_thd_block_timeout(spdid_t c, thdid_t dep_t, cycles_t abs_timeout);

thdid_t schedmgr_thd_create(spdid_t c, cos_thd_fn_t fn, void *data);
thdid_t schedmgr_aep_create(spdid_t c, struct cos_aep_info *aep, cos_aepthd_fn_t fn, void *data, int owntc);

int schedmgr_thd_param_set(spdid_t c, thdid_t tid, sched_param_t p);

int schedmgr_thd_delete(spdid_t c, thdid_t tid);
int schedmgr_thd_exit(spdid_t c);

/* TODO: lock i/f */

#endif /* SCHEDMGR_H */
