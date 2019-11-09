#include <cos_defkernel_api.h>
#include <cos_kernel_api.h>
#include "eos_pkt.h"
#include "ninf.h"
#include "ninf_util.h"
#include "eos_mca.h"

#define TX_NUM_MBUFS 8192/* 1024 */
#define TX_MBUF_DATA_SIZE 0
#define SPINTIME 60
#define TX_MBUF_SIZE (TX_MBUF_DATA_SIZE + RTE_PKTMBUF_HEADROOM + sizeof(struct rte_mbuf))

int missed;
int print_flag;
unsigned long long latency;

struct tx_ring {
	struct eos_ring *r;
	int state;
	struct tx_ring *next;
	int cid;
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
	
	missed = 0;
	print_flag = 1;
	latency = 0;
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
ninf_tx_add_ring(struct eos_ring *r, int cid)
{
	struct tx_ring *txr;

	txr = __ring_pop(&tx_fl);
	assert(txr);
	txr->r     = r;
	txr->state = 1;
	txr->cid = cid;
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

void
ninf_tx_deadline_check(struct pkt_meta *meta)
{
	unsigned long long now;

	rdtscll(now);
	latency = latency + (now - meta->arrive);
	if (now > meta->deadline) {
		missed ++;
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
		//assert(scache.cnt);
		for(i=0; i<scache.cnt; i++) {
			ninf_tx_add_pkt(nf_ring, (struct eos_ring_node *)sent, &(scache.pkts[i]));
#ifdef EOS_EDF
			ninf_tx_deadline_check(&(scache.pkts[i]));
#endif
		}

		if (scache.cnt != 0)
			sent->state = PKT_TXING;
		else
			sent->state = PKT_SENT_DONE;

		nf_ring->mca_head++;
		ret++;
	}
	if (ret) cos_faa(&(nf_ring->pkt_cnt), -ret);
	ninf_tx_flush();
	return ret;
}

//extern struct mca_info global_info[2048 + EOS_MAX_CHAIN_NUM];
static inline int
ninf_tx_scan(struct tx_ring **p)
{
	struct tx_ring *c;
	int ret = 0, t;
	cycles_t now, deadline, arrive;

	c = ps_load(p);
	while (c) {
		if (likely(c->state)) {
			p = &(c->next);
			t = ninf_tx_process(c->r);
			ret += t;
/*#ifdef EOS_EDF
			if (likely(t > 0)) {
				rdtscll(now);
				deadline = mca_info_dl_get(c->cid);
				arrive = mca_info_recv_get(c->cid);
				if (deadline < now) missed ++;
				latency = (now - arrive) + latency;
			}
#endif*/
		} else {
			*p = c->next;
			__ring_push(&tx_fl, c);
		}
		c = *p;
	}
	return ret;
}

static struct rte_eth_stats stats;
extern unsigned long long *test_rx;
extern int rx_drop_cnt;

void
ninf_info_print(int tot_rx)
{
	int port, cnt, ret;
	double rate;
	unsigned long long lat;

	for (port = 0; port < 1; port++) {
		ret = rte_eth_stats_get(port, &stats);
		assert(!ret);
		//if (stats.imissed != 0 || tot_rx != 0) {
		cnt = cos_faa(&rx_drop_cnt, 0);
		//if (cnt != 0 || stats.imissed != 0) {
			//assert(tot_rx != 0);
		printc("----------------------------------------\n");
		printc("      DROPEED BY DKDP/HW:     %llu\n", stats.imissed);
		printc("      MBUF ALLOC FAILED:      %llu\n", stats.rx_nombuf);
		printc("      DROPPED BY RX:          %d\n", cnt);
		printc("      NUM OF MISSED DEADLINE: %d\n", missed + cnt);
		printc("      TOTAL SENT:             %d\n", tot_rx);
		if (tot_rx > 0) {
			double rate = (double)(missed+cnt) / (double)(tot_rx+cnt);
			lat = latency / tot_rx;
			lat = lat / (double)2700;
			printc("      MISSED RATE:            %lf%%\n", rate*100);
			printc("      AVG LATENCY:            %llu\n", lat);
		}
		//}
	}
}

void
ninf_tx_loop()
{
	int tot_rx = 0;
	int port, ret, cnt;
	unsigned long long start, end;

	start = end = ps_tsc();
	cnt = 0;
	while(1) {
		end = ps_tsc();
		tot_rx += ninf_tx_scan(&tx);
		if ((end - start) > (unsigned long long) 2700 * (unsigned long long)20000000 && cnt < 5) {
			ninf_info_print(tot_rx);
			start = ps_tsc();
			cnt++;
		}
		//	for (port = 0; port < NUM_NIC_PORTS; port++) {
		//		ret = rte_eth_stats_get(port, &stats);
		//		if (stats.imissed != 0) {
					//printc("      DROPED BY HW:    %llu\n", stats.imissed);
					//printc("      MBUF ALLOC FAIL: %llu\n", stats.rx_nombuf);
		//		}
		//	}
			/*if (cnt < 100 && cnt > 0) {
				printc("      rx_loop:         %llu\n", test_rx[cnt-1]);
			}*/
		//	cnt++;
		//	start1 = end1 = ps_tsc();
		//}
		/*if ((end - start) > (unsigned long long)SPINTIME * (unsigned long long)2700 * (unsigned long long)1000000 && print_flag){
			double rate = missed / tot_rx;
			printc("MISS DEADLINE PERCENTAGE: %lf, MAKED NUM: %d, total : %d\n", rate, (tot_rx - missed), tot_rx);
			print_flag = 0;
			printc("PKT DROP INFO:\n");
			for(port = 0; port < NUM_NIC_PORTS; port++) {
				printc("   PORT NUM: %d\n", port);
				ret = rte_eth_stats_get(port, &stats);
				printc("      process done\n");
				assert(!ret);
				printc("      DROPED BY HW:        %llu\n", stats.imissed);
				printc("      ERROR RECVED PKT:    %llu\n", stats.ierrors);
				printc("      TRANSMIT FAILED PKT: %llu\n", stats.oerrors);
			}
		}*/
	}
}
