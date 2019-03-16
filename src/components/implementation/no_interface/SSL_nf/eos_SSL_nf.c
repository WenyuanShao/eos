#include <eos_pkt.h>
#include <nf_hypercall.h>

#include <lwip/init.h>
#include <lwip/netif.h>
#include <lwip/tcp.h>
#include <lwip/stats.h>

#define ETHER_ADDR_LEN 6
static int conf_file_idx = 0;
static const u16_t port = 0x01BB; // 443
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

static void
eos_lwip_tcp_err(void *arg,  err_t err)
{
	return;
}

static err_t
eos_lwip_tcp_recv(void *arg, struct tcp_pcb *tp, struct pbuf *p, err_t err)
{
	return ERR_OK;
}

static err_t
eos_lwip_tcp_accept(void *arg, struct tcp_pcb *tp, err_t err)
{
	struct tcp_pcb_listen *lpcb = (struct tcp_pcb_listen*)arg;
	//printc("!!!!!!!!!!!!!!!\n");
	tcp_accepted(lpcb);
	tcp_recv(tp, eos_lwip_tcp_recv);
	tcp_err(tp, eos_lwip_tcp_err);
	return ERR_OK;
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
	//temp = curr_pkt + sizeof(struct ether_addr);
	//ori_iphdr = (struct ip4_hdr *) (pkt + sizeof(struct ether_hdr));
	//printc("WWWWWWWWWWWW_src: %d\n" ,iphdr->src_addr);
	//printc("WWWWWWWWWWWW_dst: %d\n" ,iphdr->dst_addr);

	/* generate new ether_hdr*/
	ether_addr_copy(&eth_src, &eth_hdr->src_addr);
	ether_addr_copy(&eth_dst, &eth_hdr->dst_addr);

	memcpy((snd_pkt + sizeof(struct ether_hdr)), pl, p->len);
	//printc("YYYYYYYYYYYY_src: %d\n" ,((struct ip4_hdr *)(curr_pkt + sizeof(struct ether_hdr)))->src_addr);
	//printc("YYYYYYYYYYYY_dst: %d\n" ,((struct ip4_hdr *)(curr_pkt + sizeof(struct ether_hdr)))->dst_addr);
	//printc("packet regenerated\n");

	r = eos_pkt_send(output_ring, snd_pkt, len, port);
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
	/*if (IP_DEBUG | LWIP_DBG_LEVEL_WARNING & LWIP_DBG_ON) {
		printc("^^^^^^^^^\n");
	}*/
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
	//tcp_sent(tp, eos_lwip_tcp_recv);

	//printc("|||||: %d\n", tp->state);
	assert(tp != NULL);
	//printc("...........: %d\n", new_tp);
	new_tp = tcp_listen_with_backlog(tp, queue);
	if (NULL == new_tp) {
		assert(0);
	}
	//printc("^^^^^: %d\n", new_tp->state);
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
	//printc("????????: %d\n", ((struct ip4_hdr *)pl)->dst_addr);
	//if (cos_if.input(p, &cos_if) != ERR_OK) {
		//printc("failed in ip_input: %d\n", cos_if.input(p, &cos_if));
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
ssl_get_packet(int *len, int *port)
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
		struct ether_hdr *eth_hdr = (struct ether_hdr*)pkt;
		eth_copy = 1;
		ether_addr_copy(&eth_hdr->src_addr, &eth_src);
		ether_addr_copy(&eth_hdr->dst_addr, &eth_dst);
	}
	return pkt;
}

static void
ssl_server_run() {
	int len, port, r;
	void *pkt;
	
	while(1) {
		pkt = ssl_get_packet(&len, &port);

		//printc("YYYYYYYYYYYY_src: %d\n" ,((struct ip4_hdr *)(pkt + sizeof(struct ether_hdr)))->src_addr);
		//printc("YYYYYYYYYYYY_dst: %d\n" ,((struct ip4_hdr *)(pkt + sizeof(struct ether_hdr)))->dst_addr);
		
		assert(pkt);
		assert(len <= EOS_PKT_MAX_SZ);
		if (input_ring->cached.idx == 1) output_ring->cached.cnt = EOS_PKT_PER_ENTRY; /* this is a new batch */
		//printc("in ssl server\n %s", pkt);
		cos_net_interrupt(len, pkt);
		assert(len < EOS_PKT_MAX_SZ);
		if (input_ring->cached.idx == input_ring->cached.cnt) { /* the end of this batch */
			output_ring->cached.cnt = output_ring->cached.idx;
			eos_pkt_send_flush(output_ring);
		}
		printc("------------------------------\n\n");
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
