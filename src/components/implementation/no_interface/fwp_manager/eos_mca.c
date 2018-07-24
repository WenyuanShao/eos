#include <stdlib.h>
#include <string.h>
#include <cos_kernel_api.h>
#include "eos_mca.h"
#include "eos_pkt.h"

#define MCA_CONN_MAX_NUM 1024
#define MCA_BURST_CNT (32/EOS_PKT_PER_ENTRY)

static struct mca_conn *fl, *lh;

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

static inline int
mca_process(struct mca_conn *conn, int burst)
{
	struct eos_ring *src, *dst;
	volatile struct eos_ring_node *rn, *sn;
	struct eos_ring_node rcache, scache;
	int fh, r = 0;

loop:
	src = conn->src_ring;
	dst = conn->dst_ring;
	sn  = GET_RING_NODE(src, src->mca_head & EOS_RING_MASK);
	if (unlikely(sn->state != PKT_SENT_READY)) return -1;
	assert(sn->cnt);

	/* fh  = cos_faa(&(dst->free_head), 0); */
	fh  = dst->free_head;
	rn  = GET_RING_NODE(dst, fh & EOS_RING_MASK);
	if (unlikely(rn->state != PKT_FREE)) return -1;
	assert(!rn->cnt);

	scache = *sn;
	rcache = *rn;
	mca_transfer(&scache, &rcache);
	sn->state = PKT_SENT_DONE;
	*rn       = rcache;

	dst->free_head++;
	src->mca_head++;
	if ((++r) < burst) goto loop;
	/* src->pkt_cnt--; */
	/* fh = cos_faa(&(get_output_ring((void *)dst)->pkt_cnt), 1); */
	/* printc("M\n"); */
	/* fh = cos_faa(&(dst->pkt_cnt), 1); */
	/* if (!fh) { */
	/* 	eos_thd_wakeup(dst->coreid, dst->thdid); */
	/* } */

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
mca_conn_create(struct eos_ring *src, struct eos_ring *dst)
{
	struct mca_conn *conn;

	conn = mca_conn_alloc();
	assert(conn);
	conn->src_ring = src;
	conn->dst_ring = dst;
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
	for(i=0; i<MCA_CONN_MAX_NUM-1; i++) {
		fl[i].next = &fl[i+1];
	}
}
