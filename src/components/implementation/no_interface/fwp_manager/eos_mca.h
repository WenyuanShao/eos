#ifndef __EOS_MCA_H__
#define __EOS_MCA_H__

#include "eos_ring.h"
#include <ck_ring.h>

#define EOS_EDF

//#ifdef EOS_EDF
//#undef EOS_EDF
//#endif

struct mca_conn {
	struct eos_ring *src_ring;
	struct eos_ring *dst_ring;
	struct mca_conn *next;
	struct mca_info *src_info;
	struct mca_info *dst_info;
	int used;
	int src_ip, src_port;
	int dst_ip, dst_port;
};

struct mca_op {
	thdid_t tid;
	vaddr_t shmem_addr;
};

enum dl_ring_state {
	DL_RING_FREE = 0,
	DL_RING_USED,
};

struct dl {
	int      state;
	cycles_t dl;
};

struct mca_info {
	cycles_t            deadline;
	int                 info_idx;
	int                 used;
	int                 empty_flag;
	int                 head;
	int                 tail;
	struct dl          *dl_ring;
};

#define MCA_XCPU_RING_SIZE (64 * sizeof(struct mca_op))

struct mca_global {
	struct ck_ring mca_ring[NUM_CPU];
	struct mca_op  mca_rbuf[NUM_CPU][MCA_XCPU_RING_SIZE];
} CACHE_ALIGNED;

struct mca_conn *mca_conn_create(struct eos_ring *src, struct eos_ring *dst, int src_thdid, int dst_thdid);
void mca_conn_free(struct mca_conn *conn);
void mca_init(struct cos_compinfo *parent_cinfo);
void mca_run(void *d);
int mca_dl_enqueue(int cid, cycles_t deadline, int test);
cycles_t mca_info_dl_get(int cid);
cycles_t mca_info_recv_get(int cid);

CK_RING_PROTOTYPE(mca, mca_op);
extern struct mca_global mca_global_data;

static inline struct mca_global *
__get_mca_global_data(void)
{
	return &mca_global_data;
}

static inline struct ck_ring *
__mca_get_ring(cpuid_t cpu)
{
	return &(__get_mca_global_data()->mca_ring[cpu]);
}

static inline struct mca_op *
__mca_get_ring_buf(cpuid_t cpu)
{
	return (__get_mca_global_data()->mca_rbuf[cpu]);
}

int mca_xcpu_thd_dl_reset(thdid_t tid, cpuid_t cpu, vaddr_t shmem_addr);

void mca_debug_print(int cid);

#endif /* __EOS_MCA_H__ */

