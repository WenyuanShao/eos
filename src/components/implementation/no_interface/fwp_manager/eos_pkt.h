#ifndef __EOS_PKT_H__
#define __EOS_PKT_H__

#include <util.h>
#include "eos_ring.h"

#define EBLOCK  1
#define ECOLLET 2
#define ESENT   3

static inline void *
eos_pkt_allocate(struct eos_ring *ring, int len)
{
	struct eos_ring_node *cache;
	struct pkt_meta *meta;

	assert(len <= EOS_PKT_MAX_SZ);
	if (ring->cached.cnt == ring->cached.idx) {
		assert(ring->cached.alloc_idx < EOS_PKT_PER_ENTRY);
	} else {
		assert(ring->cached.alloc_idx < ring->cached.idx);
	}
	cache = &(ring->cached);
	meta  = &(cache->pkts[cache->alloc_idx]);
	cache->alloc_idx++;
	return meta->pkt;
}

/* FIXME: this does not work due to the change of eos_ring */
static inline void
eos_pkt_free(struct eos_ring *ring, void *pkt)
{
	/* volatile struct eos_ring_node *n; */
	(void)pkt;
	(void)ring;

	/* n = GET_RING_NODE(ring, ring->head & EOS_RING_MASK); */
	/* if (ps_load(&(n->state)) == PKT_EMPTY) { */
	/* 	n->pkt     = pkt; */
	/* 	n->pkt_len = EOS_PKT_MAX_SZ; */
	/* 	ps_cc_barrier(); */
	/* 	n->state   = PKT_FREE; */
	/* 	ring->head++; */
	/* } */
}

static inline int
eos_pkt_send_flush(struct eos_ring *ring)
{
	volatile struct eos_ring_node *rn;
	pkt_states_t state;

	assert(ring->cached.cnt <= EOS_PKT_PER_ENTRY);
	if (ring->cached.cnt == 0) return 0;
	if (ring->cached.idx < ring->cached.cnt) return -ESENT;
	rn = GET_RING_NODE(ring, ring->tail & EOS_RING_MASK);
	assert(rn);
	state = rn->state;
	if (unlikely(state == PKT_SENT_DONE)) return -ECOLLET;
	else if (unlikely(state != PKT_EMPTY)) return -EBLOCK;
	ring->cached.idx = 0;
	memcpy((void *)rn, &(ring->cached), sizeof(struct eos_ring_node));
	ring->tail++;
	ring->cached.cnt = 0;
	return 0;
}

static inline int
eos_pkt_send(struct eos_ring *ring, void *pkt, int len, int port)
{
	int r;
	struct eos_ring_node *cache;
	struct pkt_meta *meta;

	cache = &(ring->cached);
	if (cache->cnt == cache->idx) {
		r = eos_pkt_send_flush(ring);
		if (unlikely(r)) return r;
	}
	meta          = &(cache->pkts[cache->idx++]);
	meta->pkt     = pkt;
	meta->pkt_len = len;
	meta->port    = port;
	if (cache->cnt == cache->idx) {
		r = eos_pkt_send_flush(ring);
	}
	return 0;
}

static inline int
eos_pkt_recv_slow(struct eos_ring *ring, struct eos_ring *sent)
{
	volatile struct eos_ring_node *rn;

	assert(ring->cached.cnt == ring->cached.idx);
	while (1) {
		rn = GET_RING_NODE(ring, ring->tail & EOS_RING_MASK);
		if (rn->state != PKT_EMPTY) break;
		ring->tail++;
	}
	if (likely(rn->state == PKT_RECV_READY)) {
		memcpy(&(ring->cached), (void *)rn, sizeof(struct eos_ring_node));
		assert(ring->cached.cnt);
		assert(ring->cached.idx == 0);
		assert(sent->cached.cnt == 0);
		assert(sent->cached.idx == 0);
		memcpy(sent->cached.pkts, ring->cached.pkts, sizeof(ring->cached.pkts));
		ring->cached.alloc_idx = 0;
		sent->cached.cnt       = ring->cached.cnt;
		rn->state              = PKT_RECV_DONE;
		ring->tail++;
		/* cos_faa(&(ring->pkt_cnt), -1); */
		return 0;
	} else if (rn->state == PKT_RECV_DONE) {
		return -ECOLLET;
	} else {
		return -EBLOCK;
	}
}

static inline void *
eos_pkt_recv(struct eos_ring *ring, int *len, int *port, int *err, struct eos_ring *sent)
{
	int r;
	void *ret = NULL;
	struct eos_ring_node *cache;
	struct pkt_meta *meta;

	if (ring->cached.cnt == ring->cached.idx) {
		r =eos_pkt_recv_slow(ring, sent);
		if (r) {
			*err = r;
			return NULL;
		}
	}
	cache = &(ring->cached);
	meta = &(cache->pkts[cache->idx]);
	assert(meta->pkt);
	assert(meta->pkt_len);
	ret   = meta->pkt;
	*len  = meta->pkt_len;
	*port = meta->port;
	cache->idx++;
	/* if (ring->cached.idx < ring->cached.cnt) { */
	__builtin_prefetch(cache->pkts[cache->idx].pkt, 1);
	/* } */

	return ret;
}

static inline int
eos_pkt_collect(struct eos_ring *recv, struct eos_ring *sent)
{
	volatile struct eos_ring_node *rn, *sn;
	struct eos_ring_node *nxt;
	int r = 0;
	struct eos_ring_node scache;
	pkt_states_t state;

collect:
	rn = GET_RING_NODE(recv, recv->head & EOS_RING_MASK);
	sn = GET_RING_NODE(sent, sent->head & EOS_RING_MASK);
	state = rn->state;

	if (likely((state == PKT_EMPTY || state == PKT_RECV_DONE) && sn->state  == PKT_SENT_DONE)) {
		memcpy(&scache, (void *)sn, sizeof(struct eos_ring_node));

		/* debug */
		struct eos_ring_node rcache;
		int i;
		memcpy(&rcache, (void *)rn, sizeof(struct eos_ring_node));
		assert(recv->head == sent->head);
		for(i=0; i<EOS_PKT_PER_ENTRY; i++) {
			assert(rcache.pkts[i].pkt == scache.pkts[i].pkt);
		}
		scache.cnt   = scache.idx = 0;
		scache.state = PKT_FREE;
		sn->state    = PKT_EMPTY;
		memcpy((void *)rn, &scache, sizeof(struct eos_ring_node));
		recv->head++;
		sent->head++;
		r++;
		nxt = (struct eos_ring_node *)GET_RING_NODE(sent, (sent->head+1) & EOS_RING_MASK);
		__builtin_prefetch(nxt, 1);
		goto collect;
	}
	return r;
}

#endif /* __EOS_PKT_H__ */
