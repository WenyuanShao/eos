#include <eos_pkt.h>
#include <nf_hypercall.h>

#include <lwip/init.h>
#include <lwip/netif.h>
#include <lwip/tcp.h>
#include <lwip/stats.h>
#include <lwip/prot/tcp.h>

#define ETHER_ADDR_LEN 6
static int conf_file_idx = 0;
static const u16_t port = 0x01BB; // 443
static u16_t tx_port; // 443
static struct ip4_addr ip, mask, gw;
static struct netif cos_if;
static vaddr_t shmem_addr;
static struct eos_ring *input_ring, *output_ring;
struct ether_addr eth_src, eth_dst;
static int eth_copy = 0;
//static struct tcp_pcb_listen *lpcb;

struct ip4_hdr {
	uint8_t  version_ihl;
	uint8_t  type_of_service;
	uint16_t total_len;
	uint16_t packet_id;
	uint16_t fragment_offset;
	uint8_t  time_to_live;
	uint8_t  next_proto_id;
	uint16_t hdr_checksum;
	uint32_t src_addr;
	uint32_t dst_addr;
} __attribute__((__packed__));

struct ether_addr {
	uint8_t addr_bytes[ETHER_ADDR_LEN];
} __attribute__((__packed__));

struct ether_hdr {
	struct ether_addr dst_addr;
	struct ether_addr src_addr;
	uint16_t ether_type;
} __attribute__((__packed__));

struct echoserver_struct {
	uint8_t state;
	struct tcp_pcb *tp;
	struct pbuf *p;
};

enum echoserver_state {
	ES_NONE = 0,
	ES_ACCEPTED,
	ES_RECEIVED,
	ES_CLOSING
};

static void
ssl_print_pkt(void * pkt, int len) {
	int i = 0;
	printc("{ ");
	for (i = 0; i < len; i++) {
		printc("%02x ", ((uint8_t*)(pkt))[i]);
	}
	printc("}\n");
}

static void
eos_lwip_tcp_err(void *arg,  err_t err)
{
	return;
}

static void eos_lwip_tcp_close(struct tcp_pcb *tp, struct echoserver_struct *es) {
	tcp_arg(tp, NULL);
	tcp_sent(tp, NULL);
	tcp_recv(tp, NULL);
	tcp_err(tp, NULL);

	if (es != NULL) {
		free(es);
	}

	tcp_close(tp);
}

static void
eos_lwip_tcp_send(struct tcp_pcb *tp, struct echoserver_struct *es)
{
	struct pbuf *pchain;
	err_t wr_err = ERR_OK;

	while (wr_err == ERR_OK && (es->p != NULL) && (es->p->len <= tcp_sndbuf(tp))) {
		pchain = es->p;
		wr_err = tcp_write(tp, pchain->payload, pchain->len, 1);

		if (wr_err == ERR_OK) {
			u16_t plen;
			plen = pchain->len;
			es->p = pchain->next;
			if (es->p != NULL) {
				pbuf_ref(es->p);
			}
			pbuf_free(pchain);
			tcp_recved(tp, plen);
		} else if (wr_err == ERR_MEM){
			assert(0);
		} else assert(0);
	}
}

static err_t
eos_lwip_tcp_recv(void *arg, struct tcp_pcb *tp, struct pbuf *p, err_t err)
{
	err_t ret_err;
	struct echoserver_struct *es;

	es = (struct echoserver_struct *)arg;
	if (p == NULL) {
		es->state = ES_CLOSING;
		if (es->p == NULL){
			eos_lwip_tcp_close(tp, es);
		} else {
			eos_lwip_tcp_send(tp, es);
		}
		return ERR_OK;
	}

	if (err != ERR_OK) {
		if (p != NULL) {
			es->p = NULL;
			pbuf_free(p);
		}
		return err;
	}

	if (es->state == ES_ACCEPTED) {
		es->state = ES_RECEIVED;
		es->p = p;
		/* send back the received data */
		eos_lwip_tcp_send(tp, es);

		return ERR_OK;
	}

	if (es->state == ES_RECEIVED) {
		if (es->p == NULL) {
			es->p = p;
			/* send back the received data */
			eos_lwip_tcp_send(tp, es);
		} else {
			struct pbuf *pchain;

			pchain = es->p;
			pbuf_chain(pchain, p);
		}
		return ERR_OK;
	}
	tcp_recved(tp, p->tot_len);
	es->p = NULL;
	pbuf_free(p);
	return ERR_OK;
}

static err_t
eos_lwip_tcp_sent(void *arg, struct tcp_pcb *tp, u16_t len)
{
	struct echoserver_struct *es;

	es = (struct echoserver_struct *)arg;
	if (es->p != NULL) {
		eos_lwip_tcp_send(tp, es);
	} else {
		if (es->state == ES_CLOSING) {
			eos_lwip_tcp_close(tp, es);
		}
	}
	return ERR_OK;
}

static err_t
eos_lwip_tcp_accept(void *arg, struct tcp_pcb *tp, err_t err)
{
	err_t ret_err;
	struct echoserver_struct *es;

	es = (struct echoserver_struct *)malloc(sizeof(struct echoserver_struct));
	if (es != NULL) {
		es->state = ES_ACCEPTED;
		es->tp = tp;
		es->p = NULL;

		tcp_arg(tp, es);
		tcp_err(tp, eos_lwip_tcp_err);
		tcp_recv(tp, eos_lwip_tcp_recv);
		tcp_sent(tp, eos_lwip_tcp_sent);

		ret_err = ERR_OK;
	} else {
		ret_err = ERR_MEM;
	}
	return ret_err;
}

static inline void
ether_addr_copy(struct ether_addr *src, struct ether_addr *dst)
{
	*dst = *src;
}

static err_t
ssl_output(struct netif *ni, struct pbuf *p, const ip4_addr_t *ip)
{
	void *pl;
	struct ip4_hdr *iphdr;
	struct ether_hdr *eth_hdr;
	struct ip4_hdr *ori_iphdr;
	void *snd_pkt;
	int r, len;

	pl      = p->payload;
	len     = sizeof(struct ether_hdr) + p->len;
	snd_pkt = eos_pkt_allocate(input_ring, len);
	eth_hdr = (struct ether_hdr*)snd_pkt;
	iphdr   = (struct ip4_hdr *)pl;

	iphdr = (struct ip4_hdr *)pl;

	/* generate new ether_hdr*/
	ether_addr_copy(&eth_src, &eth_hdr->src_addr);
	ether_addr_copy(&eth_dst, &eth_hdr->dst_addr);

	memcpy((snd_pkt + sizeof(struct ether_hdr)), pl, p->len);

	r = eos_pkt_send(output_ring, snd_pkt, len, tx_port);
	assert(!r);
	return ERR_OK;
}

static err_t
cos_if_init(struct netif *ni)
{
	ni->name[0] = 'c';
	ni->name[1] = 'n';
	ni->mtu     = 1500;
	ni->output  = ssl_output;

	return ERR_OK;
}

static void
init_lwip(void)
{
	lwip_init();
	//tcp_mem_free();
	IP4_ADDR(&ip, 10,10,1,1);
	IP4_ADDR(&mask, 255,255,255,0);
	IP4_ADDR(&gw, 10,10,1,1);

	netif_add(&cos_if, &ip, &mask, &gw, NULL, cos_if_init, ip4_input);
	netif_set_default(&cos_if);
	netif_set_up(&cos_if);
	netif_set_link_up(&cos_if);
}

static void
eos_create_tcp_connection()
{
	err_t ret;
	int queue;
	struct tcp_pcb *new_tp, *tp;
	
	tp = tcp_new();
	if (tp == NULL) {
		printc("Could not create tcp connection\n");
	}
	struct ip4_addr ipa = *(struct ip4_addr*)&ip;
	ret = tcp_bind(tp, &ipa, port);
	assert(ret == ERR_OK);

	assert(tp != NULL);
	new_tp = tcp_listen_with_backlog(tp, queue);
	if (NULL == new_tp) {
		assert(0);
	}
	tcp_arg(new_tp, new_tp);
	tcp_accept(new_tp, eos_lwip_tcp_accept);
}

static void
cos_net_interrupt(int len, void * pkt)
{
	void *pl;
	struct pbuf *p;
	
	pl = malloc(len);
	memcpy(pl, pkt, len);
	pl = (void *)(pl + sizeof(struct ether_hdr));
	p = pbuf_alloc(PBUF_IP, (len-sizeof(struct ether_hdr)), PBUF_ROM);
	assert(p);
	p->payload = pl;
	if (cos_if.input(p, &cos_if) != ERR_OK) {
		pbuf_free(p);
	}
	return;
}

static void
init(void)
{
	init_lwip();
	eos_create_tcp_connection();
}

void *
ssl_get_packet(int *len, u16_t *port)
{
	int err, r = 0;
	void *pkt;

	eos_pkt_collect(input_ring, output_ring);
	pkt = eos_pkt_recv(input_ring, len, port, &err, output_ring);
	while (unlikely(!pkt)) {
		if (err == -EBLOCK) nf_hyp_block();
		else if (err == -ECOLLET) eos_pkt_collect(input_ring, output_ring);
		pkt = eos_pkt_recv(input_ring, len, port, &err, output_ring);
	}
	if (unlikely(!eth_copy)) {
		struct ether_hdr *eth_hdr = (struct ether_hdr *)pkt;
		eth_copy = 1;
		ether_addr_copy(&eth_hdr->src_addr, &eth_dst);
		ether_addr_copy(&eth_hdr->dst_addr, &eth_src);
	}
	return pkt;
}

static void
ssl_server_run() {
	int len, r;
	void *pkt;
	
	while(1) {
		pkt = ssl_get_packet(&len, &tx_port);
		assert(pkt);
		assert(len <= EOS_PKT_MAX_SZ);
		if (input_ring->cached.idx == 1) {
			output_ring->cached.cnt = EOS_PKT_PER_ENTRY + 1;
		}
		cos_net_interrupt(len, pkt);
		assert(len < EOS_PKT_MAX_SZ);
		if (input_ring->cached.idx == input_ring->cached.cnt) {
			/* the end of this batch */
			output_ring->cached.cnt = output_ring->cached.idx;
			eos_pkt_send_flush_force(output_ring);
		}
		// application function.
	}
}

void
cos_init(void *args) 
{
	nf_hyp_confidx_get(&conf_file_idx);
	nf_hyp_get_shmem_addr(&shmem_addr);

	input_ring  = get_input_ring((void *)shmem_addr);
	output_ring = get_output_ring((void *)shmem_addr);
	if (conf_file_idx == -1) {
		ssl_server_run();
	} else {
		init();
		printc("ssl server pre populate done\n");
		nf_hyp_checkpoint(cos_spd_id());
		ssl_server_run();
	}
}
