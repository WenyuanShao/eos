#include <cos_defkernel_api.h>
#include <cos_kernel_api.h>
#include "eos_pkt.h"
#include "ninf.h"
#include "ninf_util.h"

#define TX_NUM_MBUFS 8192/* 1024 */
#define TX_MBUF_DATA_SIZE 0
#define TX_MBUF_SIZE (TX_MBUF_DATA_SIZE + RTE_PKTMBUF_HEADROOM + sizeof(struct rte_mbuf))

struct tx_ring {
	struct eos_ring *r;
	int state;
	struct tx_ring *next;
};

struct tx_pkt_batch {
	struct eos_ring_node *rn;
	struct pkt_meta *meta;
	void *phy_addr; 	/* physical address of pkt in this meta */
};

static struct tx_ring *tx, *tx_fl;
static struct tx_ring tx_rings[EOS_MAX_CHAIN_NUM];
static burst_cnt[NUM_NIC_PORTS];
static struct tx_pkt_batch send_batch[NUM_NIC_PORTS][BURST_SIZE];
static struct rte_mempool *tx_mbuf_pool;
static struct rte_mbuf *tx_batch_mbufs[BURST_SIZE] = {NULL};

static inline void
__ring_push(struct tx_ring **h, struct tx_ring *n)
{
	struct tx_ring *t;

	do {
		t       = ps_load(h);
		n->next = t;
	} while (!cos_cas((unsigned long *)h, (unsigned long)t, (unsigned long)n));
}

static inline struct tx_ring *
__ring_pop(struct tx_ring **h)
{
	struct tx_ring *r, *t;

	do {
		r = ps_load(h);
		assert(r);
		t = r->next;
	} while (!cos_cas((unsigned long *)h, (unsigned long)r, (unsigned long)t));
	return r;
}

void
ninf_tx_init()
{
	int i;
	
	tx_fl = tx_rings;
	for(i=0; i<EOS_MAX_CHAIN_NUM-1; i++) {
		tx_fl[i].next = &tx_fl[i+1];
	}
	tx_fl[i].next = NULL;
	tx = NULL;
	for(i=0; i<NUM_NIC_PORTS; i++) burst_cnt[i] = 0;
	tx_mbuf_pool = rte_pktmbuf_pool_create("TX_MBUF_POOL", TX_NUM_MBUFS * NUM_NIC_PORTS, MBUF_CACHE_SIZE, 0, TX_MBUF_SIZE, -1);
}

struct tx_ring *
ninf_tx_add_ring(struct eos_ring *r)
{
	struct tx_ring *txr;

	txr = __ring_pop(&tx_fl);
	assert(txr);
	txr->r     = r;
	txr->state = 1;
	__ring_push(&tx, txr);
	return txr;
}

void
ninf_tx_del_ring(struct tx_ring *r)
{
	r->state = 0;
}

extern struct rte_mempool *rx_mbuf_pool;
static inline void
ninf_tx_nf_send_burst(struct tx_pkt_batch *batch, int port)
{
	int i, cnt, nb_tx, left;
	struct rte_mbuf **mbatch;

	cnt = burst_cnt[port];
	if (!cnt) return ;

	if (rte_pktmbuf_alloc_bulk(tx_mbuf_pool, tx_batch_mbufs, cnt)) {
		assert(0);
	}
	for(i=0; i<cnt; i++) {
		tx_batch_mbufs[i]->buf_addr     = batch[i].meta->pkt;
		tx_batch_mbufs[i]->buf_physaddr = (uint64_t)batch[i].phy_addr;
		tx_batch_mbufs[i]->userdata     = batch[i].rn;
		tx_batch_mbufs[i]->data_len     = (uint16_t)batch[i].meta->pkt_len;
		tx_batch_mbufs[i]->pkt_len      = (uint16_t)batch[i].meta->pkt_len;
		assert(batch[i].meta->pkt_len <= EOS_PKT_MAX_SZ);
		tx_batch_mbufs[i]->data_off     = 0;
	}

	left = cnt;
	mbatch = tx_batch_mbufs;
	while (left > 0) {
		nb_tx = rte_eth_tx_burst(port, 1, mbatch, left);
		left -= nb_tx;
		mbatch += nb_tx;
	}
	burst_cnt[port] = 0;
}

static inline void *
get_phy_addr_ring_node(struct eos_ring *nf_ring, struct pkt_meta *node)
{
	return nf_ring->ring_phy_addr + (node->pkt - (void *)nf_ring);
}

static inline void
ninf_tx_add_pkt(struct eos_ring *nf_ring, struct eos_ring_node *node, struct pkt_meta *meta)
{
	int port, cnt;

	port = meta->port;
	cnt  = burst_cnt[port];
	if (cnt == BURST_SIZE) {
		ninf_tx_nf_send_burst(send_batch[port], port);
		cnt = 0;
	}
	send_batch[port][cnt].rn       = node;
	send_batch[port][cnt].meta     = meta;
	send_batch[port][cnt].phy_addr = get_phy_addr_ring_node(nf_ring, meta);
	burst_cnt[port] = cnt + 1;
}

static inline void
ninf_tx_flush()
{
	int i;

	for(i=0; i<NUM_NIC_PORTS; i++) {
		ninf_tx_nf_send_burst(send_batch[i], i);
	}
}

static inline int
ninf_tx_process(struct eos_ring *nf_ring)
{
	int ret = 0, r, i;
	volatile struct eos_ring_node *sent;
	struct eos_ring_node scache;
	
	while (1) {
		sent = GET_RING_NODE(nf_ring, nf_ring->mca_head & EOS_RING_MASK);
		if (unlikely(sent->state != PKT_SENT_READY)) break ;
		
		scache = *sent;
		assert(scache.cnt);
		for(i=0; i<scache.cnt; i++) {
			ninf_tx_add_pkt(nf_ring, (struct eos_ring_node *)sent, &(scache.pkts[i]));
		}
		sent->state = PKT_TXING;
		nf_ring->mca_head++;
		ret++;
	}
	if (ret) cos_faa(&(nf_ring->pkt_cnt), -ret);
	ninf_tx_flush();
	return ret;
}

static inline int
ninf_tx_scan(struct tx_ring **p)
{
	struct tx_ring *c;
	int ret = 0;

	c = ps_load(p);
	while (c) {
		if (likely(c->state)) {
			p = &(c->next);
			ret += ninf_tx_process(c->r);
		} else {
			*p = c->next;
			__ring_push(&tx_fl, c);
		}
		c = *p;
	}
	return ret;
}

void
ninf_tx_loop()
{
	int tot_rx = 0;

	while(1) {
		tot_rx += ninf_tx_scan(&tx);
	}
}
