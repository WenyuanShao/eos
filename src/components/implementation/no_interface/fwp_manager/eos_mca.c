#include <stdlib.h>
#include <string.h>
#include <cos_kernel_api.h>
#include <sl.h>

#include "eos_mca.h"
#include "eos_pkt.h"
#include "eos_utils.h"

#define MCA_CONN_MAX_NUM 2048

#define OFFSET_US        1000

#define MCA_BURST_CNT    (32/EOS_PKT_PER_ENTRY)

static struct mca_conn *fl, *lh;
static volatile struct mca_info global_info[MCA_CONN_MAX_NUM + EOS_MAX_CHAIN_NUM];
int info_idx = 0;
struct mca_global mca_global_data;

int skip_cnt = 0;
int
mca_xcpu_process(void)
{
	struct mca_op op;
	struct eos_ring *output_ring;
	int num = 0, cpu, idx;

	for (cpu = NF_MIN_CORE; cpu < NF_MAX_CORE; cpu++) {
		while (ck_ring_dequeue_mpsc_mca(__mca_get_ring(cpu), __mca_get_ring_buf(cpu), &op) == true) {
			assert(op.tid < MCA_CONN_MAX_NUM);
			idx = op.tid + EOS_MAX_CHAIN_NUM;
			output_ring = get_output_ring((void *)op.shmem_addr);
			if (output_ring->pkt_cnt > 0) {
				global_info[idx].empty_flag = 1;
				continue;
			}
			//printc("SET TO MAX DEADLINE\n");
			global_info[idx].deadline = EOS_MAX_DEADLINE;
			num++;
		}
	}
	return num;
}

volatile struct mca_info *
mca_info_get(int tid)
{
	volatile struct mca_info *ret;

	assert(tid < MCA_CONN_MAX_NUM + EOS_MAX_CHAIN_NUM);

	ret = &global_info[tid];
	ret->info_idx = tid;
	if (tid < EOS_MAX_CHAIN_NUM) {
		assert(!ret->used);
		ret->info_idx = tid;
		ret->deadline = EOS_MAX_DEADLINE;
		ret->used = 1;
		return ret;
	}
	if (!ret->used) {
		ret->info_idx = tid;
		ret->deadline = EOS_MAX_DEADLINE;
		ret->used = 1;
	}
	return ret;
}

static inline void
mca_push_l(struct mca_conn **l, struct mca_conn *c)
{
	struct mca_conn *t;

	do {
		t       = ps_load(l);
		c->next = t;
	} while (!cos_cas((unsigned long *)l, (unsigned long)t, (unsigned long)c));
}

static inline struct mca_conn *
mca_conn_alloc(void)
{
	struct mca_conn *r, *t;

	assert(fl);
	do {
		r = ps_load(&fl);
		t = r->next;
	} while (!cos_cas((unsigned long *)&fl, (unsigned long)r, (unsigned long)t));
	return r;
}

static inline void
mca_conn_collect(struct mca_conn *conn)
{
	mca_push_l(&fl, conn);
}

static inline void
mca_copy(void *dst, void *src, int sl)
{
	memcpy(dst, src, sl);
}

static inline void
mca_transfer(struct eos_ring_node *sn, struct eos_ring_node *rn)
{
	int i;
	const int cnt = sn->cnt-1;
	struct pkt_meta *rmeta, *smeta;

	rn->deadline = sn->deadline;
	rn->arrive   = sn->arrive;
	for(i=0; i<cnt; i++) {
		__builtin_prefetch(sn->pkts[i+1].pkt, 0);
		__builtin_prefetch(rn->pkts[i+1].pkt, 1);
		smeta           = &(sn->pkts[i]);
		rmeta           = &(rn->pkts[i]);
		assert(smeta->pkt_len <= EOS_PKT_MAX_SZ);
		rmeta->pkt_len  = smeta->pkt_len;
		rmeta->port     = smeta->port;
		mca_copy(rmeta->pkt, smeta->pkt, smeta->pkt_len);
	}
	smeta           = &(sn->pkts[cnt]);
	rmeta           = &(rn->pkts[cnt]);
	rmeta->pkt_len  = smeta->pkt_len;
	rmeta->port     = smeta->port;
	mca_copy(rmeta->pkt, smeta->pkt, smeta->pkt_len);

	rn->cnt   = sn->cnt;
	rn->state = PKT_RECV_READY;
}

unsigned long long nnnn;
int test = 0;
static inline int
mca_process(struct mca_conn *conn, int burst)
{
	struct eos_ring *src, *dst, *dst_output, *dst_input;
	struct mca_info *dst_info, *src_info;
	volatile struct eos_ring_node *rn, *sn;
	struct eos_ring_node rcache, scache;
	int fh1, fh, r = 0;
	thdid_t dst_tid;
	unsigned long long start, end;
	int pkt_cnt, ret, cnter=0;
	unsigned long long tmp = (unsigned long long)500 * (unsigned long long)1000 * (unsigned long long)2100;
	unsigned long long curr;
	int set = 0;

	start = ps_tsc();
loop:
	src = conn->src_ring;
	dst = conn->dst_ring;
	sn  = GET_RING_NODE(src, src->mca_head & EOS_RING_MASK);
#ifdef EOS_EDF
	src_info = conn->src_info;
	dst_info = conn->dst_info;
#endif
	if (unlikely(sn->state != PKT_SENT_READY)) {
		cnter++;
		assert(cnter <= 1);
		return -1;
	}

	/* fh  = cos_faa(&(dst->free_head), 0); */
	fh  = dst->free_head;
	rn  = GET_RING_NODE(dst, fh & EOS_RING_MASK);
	if (unlikely(rn->state != PKT_FREE)) {
		return -1;
	}

	if (sn->cnt == 0) {
		assert(0);
		sn->state = PKT_SENT_DONE;
		src->mca_head++;
		return -1;
	}
	//assert(sn->cnt);
	assert(!rn->cnt);

	scache = *sn;
	rcache = *rn;

#ifdef EOS_EDF
	int num = mca_xcpu_process();
	dst_output = get_output_ring(dst);
	dst_input = get_input_ring(dst);
	if (dst_info->empty_flag) {
		if (dst_output->pkt_cnt == 0 && dst_input->pkt_cnt == 0) {
			dst_info->deadline = EOS_MAX_DEADLINE;
			dst_info->empty_flag = 0;
		}
	}

	if (src_info->info_idx < EOS_MAX_CHAIN_NUM) {
		src_info->deadline = scache.deadline;
		assert(src_info->deadline < EOS_MAX_DEADLINE);
		curr = ps_tsc();
		assert(src_info->deadline <= (tmp+curr));
	}

	assert(src_info->deadline);
	/* this is a hack */
	if (src_info->deadline == EOS_MAX_DEADLINE) {
		src_info->deadline = scache.deadline;
	}
	assert(src_info->deadline < EOS_MAX_DEADLINE);

	if (src_info->info_idx >= EOS_MAX_CHAIN_NUM && src_info->deadline > dst_info->deadline) {
		return -1;
	}

	if ((src_info->deadline - (unsigned long long)OFFSET_US * (unsigned long long)2100) > dst_info->deadline || r>1) {
		return -1;
	}
	assert(r <= 1);
#endif
	mca_transfer(&scache, &rcache);
	sn->state = PKT_SENT_DONE;
	*rn       = rcache;

	dst->free_head++;
	src->mca_head++;

	fh1 = cos_faa(&(src->pkt_cnt), -sn->cnt);
	fh = cos_faa(&(dst->pkt_cnt), sn->cnt);

#ifdef EOS_EDF

	dst_tid = dst_info->info_idx - EOS_MAX_CHAIN_NUM;
	if (dst_info->deadline > src_info->deadline) {
		
		assert(src_info->deadline < EOS_MAX_DEADLINE);
		assert(dst_info->deadline == EOS_MAX_DEADLINE);
		dst_info->deadline = src_info->deadline;

	 	assert(dst_info->deadline);

		curr = ps_tsc();
		assert(dst_info->deadline <= (curr+tmp));
		ret = eos_thd_wakeup_with_dl(dst->coreid, dst_tid, dst_info->deadline);
		assert(ret == 0);
	} else {
		curr = ps_tsc();
		assert(src_info->deadline != EOS_MAX_DEADLINE);
		assert(dst_info->deadline != EOS_MAX_DEADLINE);
		assert(dst_info->deadline <= (curr+tmp));
		ret = eos_thd_wakeup_with_dl(dst->coreid, dst_tid, dst_info->deadline);
		//printc("need wakeup\n");
		assert(ret == 0);
	}
#endif

	end = ps_tsc();
	if ((++r) < burst) {
		goto loop;
	}
	return r;
}

static inline void
mca_scan(struct mca_conn **list)
{
	struct mca_conn **p, *c;

	p = list;
	c = ps_load(p);
	while (c) {
		if (likely(c->used)) {
			p = &(c->next);
			mca_process(c,  MCA_BURST_CNT);
		} else {
			*p = c->next;
			mca_conn_collect(c);
		}
		c = *p;
	}
}

void
mca_run(void *d)
{
	while (1) {
		mca_scan(&lh);
	}
}

struct mca_conn *
mca_conn_create(struct eos_ring *src, struct eos_ring *dst, int src_thdid, int dst_thdid)
{
	volatile struct mca_conn *conn;

	conn = mca_conn_alloc();
	assert(conn);
	conn->src_ring = src;
	conn->dst_ring = dst;
#ifdef EOS_EDF
	conn->src_info = mca_info_get(src_thdid);
	assert(conn->src_info->info_idx == src_thdid);
	conn->dst_info = mca_info_get(dst_thdid);
	assert(conn->dst_info->info_idx == dst_thdid);
#endif
	conn->used     = 1;
	mca_push_l(&lh, conn);
	return conn;
}

void
mca_conn_free(struct mca_conn *conn)
{
	conn->used = 0;
}

void
mca_init(struct cos_compinfo *parent_cinfo)
{
	int i, total_conn_sz;

	lh = NULL;
	total_conn_sz = sizeof(struct mca_conn) * MCA_CONN_MAX_NUM;

	fl = (struct mca_conn *)cos_page_bump_allocn(parent_cinfo, total_conn_sz);
	assert(fl);
	memset(fl, 0, total_conn_sz);

	for (i=0; i<MCA_CONN_MAX_NUM-1; i++) {
		fl[i].next = &fl[i+1];
	}
	for (i = NF_MIN_CORE; i < NF_MAX_CORE; i++) {
		ck_ring_init(__mca_get_ring(i), MCA_XCPU_RING_SIZE);
	}
	memset(global_info, 0, sizeof(struct mca_info) * (EOS_MAX_CHAIN_NUM + MCA_CONN_MAX_NUM));
}
