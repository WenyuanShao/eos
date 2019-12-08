#include <stdlib.h>
#include <string.h>
#include <cos_kernel_api.h>
#include <sl.h>

#include "eos_mca.h"
#include "eos_pkt.h"
#include "eos_utils.h"

#define MCA_CONN_MAX_NUM 2048
/* 100us */
#define OFFSET           0

#define MCA_BURST_CNT    (32/EOS_PKT_PER_ENTRY)

static struct mca_conn *fl, *lh;
static struct mca_info global_info[MCA_CONN_MAX_NUM + EOS_MAX_CHAIN_NUM];
int info_idx = 0;
struct mca_global mca_global_data;

/*static inline int
__mca_xcpu_op_exit(struct mca_op *op, cpuid_t cpu)
{
    int ret = 0;
    if (ck_ring_enqueue_mpsc_mca(__mca_get_ring(cpu), __mca_get_ring_buf(cpu), op) != true) {
        ret = -ENOMEM;
        assert(0);
    }
    sl_cs_exit();
    return ret;
}

//extern struct click_info;

int
mca_xcpu_thd_dl_reset(thdid_t tid, cpuid_t cpu)
{
	struct mca_op op;

	sl_cs_enter();

	op.tid = tid;
	//printc("\t^^^^^^: reset in core %d, tid: %d\n", cpu, tid);
	return __mca_xcpu_op_exit(&op, cpu);
}*/

cycles_t
mca_info_dl_get(int cid)
{
	struct mca_info *ret;

	ret = &global_info[cid];
	return ret->dl_ring[ret->tail & EOS_RING_MASK].dl;
}

cycles_t
mca_info_recv_get(int cid)
{
	struct mca_info *ret;

	ret = &global_info[cid];
	return ret->dl_ring[ret->tail & EOS_RING_MASK].arrive;
}

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
		assert(!ret->used);
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

int
mca_dl_enqueue(int cid, cycles_t deadline, int test)
{
	int head, tail;
	cycles_t arrive = 0;
	rdtscll(arrive);

	if (cid > EOS_MAX_CHAIN_NUM) {
		printc("cid: %d\n", cid);
	}
	assert(cid < EOS_MAX_CHAIN_NUM);

	struct mca_info* curr_info = &global_info[cid];
	assert((curr_info->head - curr_info->tail) <= EOS_RING_SIZE + 1);
	curr_info->dl_ring[curr_info->head & EOS_RING_MASK].dl = deadline + arrive;
	curr_info->dl_ring[curr_info->head & EOS_RING_MASK].arrive = arrive;
	//curr_info->deadline = curr_info->dl_ring[curr_info->tail & EOS_RING_MASK].dl;
	head = cos_faa(&(curr_info->head), 1);
	tail = cos_faa(&(curr_info->tail), 0);
	//printc("\tdl_head: %d\n", head+1);
	/*if (test != head + 1) {
		printc("test: %d, head: %d\n", test, head+1);
	}
	assert(test == head+1);*/
	/* head + 1 - tail <= 127 */
	if (head - tail > 127) {
		printc("head: %d, tail: %d, cid: %d, cid-MAX_CHAIN: %d\n", head+1, tail, cid, cid-EOS_MAX_CHAIN_NUM);
		assert(0);
	}
	//assert(head - tail < 127);

	return curr_info->head;
}

void
mca_debug_print(int cid)
{
	unsigned long long curr_time;
	assert(cid < EOS_MAX_CHAIN_NUM);
	rdtscll(curr_time);
	struct mca_info* curr_info = &global_info[cid];
	printc("ring idx: %d\n", cid);
	printc("      deadline:  %llu\n", curr_info->dl_ring[curr_info->tail].dl);
	printc("      curr_time: %llu\n", curr_time);
	printc("      arrive:    %llu\n", curr_info->dl_ring[curr_info->tail].arrive);
	printc("      head_ori: %u, head: %u, tail_ori: %u, tail: %u\n",
					curr_info->head,
					(curr_info->head & EOS_RING_MASK),
					curr_info->tail,
					(curr_info->tail & EOS_RING_MASK));
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
		smeta           = &(sn->pkts[i]);
		rmeta           = &(rn->pkts[i]);
		assert(smeta->pkt_len <= EOS_PKT_MAX_SZ);
		rmeta->pkt_len  = smeta->pkt_len;
		rmeta->port     = smeta->port;
		rmeta->deadline = smeta->deadline;
		rmeta->arrive   = smeta->arrive;
		mca_copy(rmeta->pkt, smeta->pkt, smeta->pkt_len);
	}
	smeta           = &(sn->pkts[cnt]);
	rmeta           = &(rn->pkts[cnt]);
	rmeta->pkt_len  = smeta->pkt_len;
	rmeta->port     = smeta->port;
	rmeta->deadline = smeta->deadline;
	rmeta->arrive   = smeta->arrive;
	mca_copy(rmeta->pkt, smeta->pkt, smeta->pkt_len);

	rn->cnt   = sn->cnt;
	rn->state = PKT_RECV_READY;
}

int test = 0;
static inline int
mca_process(struct mca_conn *conn, int burst)
{
	struct eos_ring *src, *dst, *dst_output;
	volatile struct eos_ring_node *rn, *sn;
	struct eos_ring_node rcache, scache;
	int fh, r = 0;
	thdid_t dst_tid;
	unsigned long long start, end;
	int pkt_cnt, ret, dl_tail, dl_head, cnter=0;

loop:
	start = ps_tsc();
	src = conn->src_ring;
	dst = conn->dst_ring;
	sn  = GET_RING_NODE(src, src->mca_head & EOS_RING_MASK);
#ifdef EOS_EDF
	int num = mca_xcpu_process();
#endif
	if (unlikely(sn->state != PKT_SENT_READY)) {
		//assert(0);
		cnter++;
		if (cnter > 1) {
			printc("cnter: %d\n", cnter);
			assert(0);
		}
		return -1;
	}
	assert(sn->cnt);

	/* fh  = cos_faa(&(dst->free_head), 0); */
	fh  = dst->free_head;
	rn  = GET_RING_NODE(dst, fh & EOS_RING_MASK);
	if (unlikely(rn->state != PKT_FREE)) {
		//assert(0);
		return -1;
	}
	assert(!rn->cnt);

	scache = *sn;
	rcache = *rn;

#ifdef EOS_EDF
	//dl_head = cos_faa(conn->src_info->head, 0);
	//if (dl_head == conn->src_info->tail) return -1;
	dst_output = get_output_ring(dst);
	if (conn->dst_info->empty_flag) {
		if (dst_output->pkt_cnt == 0) {
			conn->dst_info->deadline = EOS_MAX_DEADLINE;
			conn->dst_info->empty_flag = 0;
		}
	}

	conn->src_info->deadline = conn->src_info->dl_ring[conn->src_info->tail & EOS_RING_MASK].dl;

	if ((conn->src_info->deadline - (unsigned long long)OFFSET * (unsigned long long)100 * (unsigned long long)2700) > conn->dst_info->deadline) {
		return -1;
		assert(0);
	}
#endif
	mca_transfer(&scache, &rcache);
	sn->state = PKT_SENT_DONE;
	*rn       = rcache;

	dst->free_head++;
	src->mca_head++;

	fh = cos_faa(&(src->pkt_cnt), -sn->cnt);
	//fh = cos_faa(&(get_output_ring((void *)dst)->pkt_cnt), 1);
	fh = cos_faa(&(dst->pkt_cnt), sn->cnt);

#ifdef EOS_EDF
	if (conn->dst_info->deadline > conn->src_info->deadline) {
		dst_tid = conn->dst_info->info_idx - EOS_MAX_CHAIN_NUM;
		conn->dst_info->deadline = conn->src_info->deadline;
		
	 	ret = eos_thd_wakeup_with_dl(dst->coreid, dst_tid, conn->dst_info->deadline);
		assert(ret == 0);
	}

	if (conn->src_info->info_idx < EOS_MAX_CHAIN_NUM) {
		assert(sn->cnt != 0);
		dl_tail = cos_faa(&(conn->src_info->tail), sn->cnt);
		dl_head  = cos_faa(&(conn->src_info->head), 0);
		//assert(dl_head >= (dl_tail + sn->cnt));
		/*if (dl_head < dl_tail + sn->cnt || dl_head - dl_tail >= 127) {
			printc("      dlhead:   %d\n", dl_head);
			printc("      dltail:   %d\n", dl_tail + sn->cnt);
			printc("      mcahead:  %d\n", src->mca_head);
			printc("      ringtail: %d\n", src->tail);
			assert(0);
		}*/
		if (conn->src_info->tail < dl_head) {
			dl_tail = conn->src_info->tail & EOS_RING_MASK;
			conn->src_info->deadline = conn->src_info->dl_ring[dl_tail].dl;
		}
	}
#endif

	end = ps_tsc();
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
{
	struct mca_conn *conn;

	conn = mca_conn_alloc();
	assert(conn);
	conn->src_ring = src;
	conn->dst_ring = dst;
#ifdef EOS_EDF
	conn->src_info = mca_info_get(src_thdid);
	conn->dst_info = mca_info_get(dst_thdid);
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
