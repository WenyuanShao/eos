#include <stdlib.h>
#include <string.h>
#include <cos_kernel_api.h>
#include "eos_mca.h"
#include "eos_pkt.h"

#define MCA_CONN_MAX_NUM 2048
#define MCA_BURST_CNT (32/EOS_PKT_PER_ENTRY)

static struct mca_conn *fl, *lh;

cycles_t
mca_info_dl_get(int cid)
{
	struct mca_info *ret;

	ret = &global_info[cid];
	return ret->deadline;
}

int
mca_xcpu_process(void)
{
	struct mca_op  op;
	int num = 0, cpu, idx;

	//printc("in mca_xcpu_process: %p, %d\n", __mca_get_ring(cos_cpuid()), cos_cpuid());

	for (cpu = NF_MIN_CORE; cpu < NF_MAX_CORE; cpu++) {
		while (ck_ring_dequeue_mpsc_mca(__mca_get_ring(cpu), __mca_get_ring_buf(cpu), &op) == true) {
			assert(op.tid < MCA_CONN_MAX_NUM);
			//printc("\tset to inif: %d\n", op.tid);
			idx = op.tid + EOS_MAX_CHAIN_NUM;
			global_info[idx].deadline = EOS_MAX_DEADLINE;
			num++;
		}
	}
	return num;
}

struct mca_info *
mca_info_get(int tid)
{
	struct mca_info *ret;

	assert(tid < MCA_CONN_MAX_NUM + EOS_MAX_CHAIN_NUM);

	ret = &global_info[tid];
	ret->info_idx = tid;
	if (tid < EOS_MAX_CHAIN_NUM) {
		ret->head = ret->tail = 0;
		ret->info_idx = tid;
		ret->deadline = EOS_MAX_DEADLINE;
		return ret;
	}
	if (!ret->used) {
		ret->info_idx = tid;
		ret->deadline = EOS_MAX_DEADLINE;
		ret->used = 1;
	}
	//ret->deadline = deadline;
	return ret;
}

int
mca_dl_enqueue(int cid, cycles_t deadline)
{
	cycles_t arrive = 0;
	rdtscll(arrive);

	assert(cid < EOS_MAX_CHAIN_NUM);

	struct mca_info* curr_info = &global_info[cid];
	assert(curr_info->head - curr_info->tail <= EOS_RING_SIZE);
	curr_info->dl_ring[curr_info->head & EOS_RING_MASK].dl = deadline + arrive;
	curr_info->dl_ring[curr_info->head & EOS_RING_MASK].arrive = arrive;
	//curr_info->deadline = curr_info->dl_ring[curr_info->tail & EOS_RING_MASK].dl;
	//printc("cid: %d, deadline: %llu\n", cid,curr_info->head++, curr_info->dl_ring[curr_info->head & EOS_RING_MASK].dl);
	cos_faa(&(curr_info->head), 1);
	//printc("mca_info->head: %d\n", curr_info->head);
	//curr_info->head++;

	return curr_info->head;
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

	for(i=0; i<cnt; i++) {
		__builtin_prefetch(sn->pkts[i+1].pkt, 0);
		__builtin_prefetch(rn->pkts[i+1].pkt, 1);
		smeta          = &(sn->pkts[i]);
		rmeta          = &(rn->pkts[i]);
		assert(smeta->pkt_len <= EOS_PKT_MAX_SZ);
		rmeta->pkt_len = smeta->pkt_len;
		rmeta->port    = smeta->port;
		mca_copy(rmeta->pkt, smeta->pkt, smeta->pkt_len);
	}
	smeta          = &(sn->pkts[cnt]);
	rmeta          = &(rn->pkts[cnt]);
	rmeta->pkt_len = smeta->pkt_len;
	rmeta->port    = smeta->port;
	mca_copy(rmeta->pkt, smeta->pkt, smeta->pkt_len);

	rn->cnt   = sn->cnt;
	rn->state = PKT_RECV_READY;
}

int test = 0;
static inline int
mca_process(struct mca_conn *conn, int burst)
{
	struct eos_ring *src, *dst;
	volatile struct eos_ring_node *rn, *sn;
	struct eos_ring_node rcache, scache;
	int fh, r = 0;
	thdid_t dst_tid;
	int dl_head, dl_tail;
	unsigned long long start, end;
	int pkt_cnt, ret;

loop:
	start = ps_tsc();
	src = conn->src_ring;
	dst = conn->dst_ring;
	sn  = GET_RING_NODE(src, src->mca_head & EOS_RING_MASK);
	int num = mca_xcpu_process();
	//end = ps_tsc();
	//if (num > 0)
		//printc("process block msg: %llu\n", (end -start));
	// test code
	/*if (test_print && test < 50) {
		if (conn->dst_info->deadline == EOS_MAX_DEADLINE) {
			printc("thdid: %d, pkt_cnt: %d, deadline: MMMMM, dl_head: %lu, dl_tail: %d, sn->state: %d\n",
							(conn->dst_info->info_idx-EOS_MAX_CHAIN_NUM), dst->pkt_cnt, conn->src_info->head, conn->src_info->tail, sn->state);
		} else {
			printc("thdid: %d, pkt_cnt: %d, deadline: %llu , dl_head: %lu, dl_tail: %d, sn->state: %d\n",
							(conn->dst_info->info_idx-EOS_MAX_CHAIN_NUM), dst->pkt_cnt, conn->dst_info->deadline, conn->src_info->head, conn->src_info->tail, sn->state);
		}
		test ++;
	}*/

	if (unlikely(sn->state != PKT_SENT_READY)) {
		//assert(0);
		return -1;
	}
	assert(sn->cnt);

	/* fh  = cos_faa(&(dst->free_head), 0); */
	fh  = dst->free_head;
	rn  = GET_RING_NODE(dst, fh & EOS_RING_MASK);
	if (unlikely(rn->state != PKT_FREE)) {
		assert(0);
		return -1;
	}
	assert(!rn->cnt);

	scache = *sn;
	rcache = *rn;
	//src_deadline = conn->src_info->deadline;
	//dst_deadline = conn->dst_info->deadline;
	conn->src_info->deadline = conn->src_info->dl_ring[conn->src_info->tail & EOS_RING_MASK].dl;
	//printc("src_info->deadline: %llu, dst_info->deadline: %llu\n", conn->src_info->deadline, conn->dst_info->deadline);
	if ((conn->src_info->deadline - (unsigned long long)OFFSET * (unsigned long long)100 * (unsigned long long)2700) > conn->dst_info->deadline) {
		/*pkt_cnt = cos_faa(&(dst->pkt_cnt), 0);
		if (pkt_cnt == 0) {
			//start = ps_tsc();
			do {
				mca_xcpu_process();
				//c++;
			} while (conn->dst_info->deadline != EOS_MAX_DEADLINE);
			assert(conn->dst_info->deadline == EOS_MAX_DEADLINE);
			//assert(0);
			//end = ps_tsc();
			//printc("      cnt: %d, time taken: %llu\n", c, (end-start));
		} else {
			return -1;
		}*/
		//printc("skip the current loop\n");
		return -1;
		assert(0);
	}

	mca_transfer(&scache, &rcache);
	sn->state = PKT_SENT_DONE;
	*rn       = rcache;

	dst->free_head++;
	src->mca_head++;
	//fh = cos_faa(&(src->pkt_cnt), -1);
	//fh = cos_faa(&(get_output_ring((void *)dst)->pkt_cnt), 1);
	pkt_cnt = cos_faa(&(dst->pkt_cnt), sn->cnt);

	//printc("src->cid: %d, dst->deadline: %llu, src->deadline: %llu\n", conn->src_info->info_idx, conn->dst_info->deadline, conn->src_info->deadline);
	//start = ps_tsc();
	if (conn->dst_info->deadline > conn->src_info->deadline) {
		dst_tid = conn->dst_info->info_idx - EOS_MAX_CHAIN_NUM;
		conn->dst_info->deadline = conn->src_info->deadline;
		
	 	ret = eos_thd_wakeup_with_dl(dst->coreid, dst_tid, conn->dst_info->deadline);
	 	//eos_thd_wakeup(dst->coreid, dst_tid);
		assert(ret == 0);
	}
	//end = ps_tsc();
	//printc("      wakeup: %llu\n", (end -start));

	if (conn->src_info->info_idx < EOS_MAX_CHAIN_NUM) {
		fh = cos_faa(&(conn->src_info->tail), sn->cnt);
		if (conn->src_info->tail < conn->src_info->head) {
			dl_tail = conn->src_info->tail & EOS_RING_MASK;
			conn->src_info->deadline = conn->src_info->dl_ring[dl_tail].dl;
		} else {
			conn->src_info->deadline = EOS_MAX_DEADLINE;
		}	
	}

	end = ps_tsc();
	//printc("mca_one loop: %llu\n", (end -start));
	if ((++r) < burst) goto loop;
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
//mca_conn_create(struct eos_ring *src, struct eos_ring *dst, int src_thdid, int dst_thdid, cycles_t deadline)
{
	struct mca_conn *conn;

	unsigned long long start,end;
	//start = ps_tsc();
	conn = mca_conn_alloc();
	//end = ps_tsc();
	//printc("mca_conn_alloc: %llu\n", (end-start));
	assert(conn);
	conn->src_ring = src;
	conn->dst_ring = dst;
	//conn->src_info = mca_info_get(src_thdid, deadline);
	conn->src_info = mca_info_get(src_thdid);
	//conn->dst_info = mca_info_get(dst_thdid, EOS_MAX_DEADLINE);
	conn->dst_info = mca_info_get(dst_thdid);
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
	//total_fwp_info_sz = sizeof(struct mca_info) * (EOS_MAX_CHAIN_NUM + EOS_MAX_NF_NUM);

	fl = (struct mca_conn *)cos_page_bump_allocn(parent_cinfo, total_conn_sz);
	assert(fl);
	memset(fl, 0, total_conn_sz);
	/*info = (struct mca_info *)cos_page_bump_allocn(parent_cinfo, total_conn_sz);
	assert(info);
	memset(info, 0, total_fwp_info_sz);*/

	for (i=0; i<MCA_CONN_MAX_NUM-1; i++) {
		fl[i].next = &fl[i+1];
	}
	for (i = NF_MIN_CORE; i < NF_MAX_CORE; i++) {
		ck_ring_init(__mca_get_ring(i), MCA_XCPU_RING_SIZE);
	}
	memset(global_info, 0, sizeof(struct mca_info) * (EOS_MAX_CHAIN_NUM + MCA_CONN_MAX_NUM));
}
