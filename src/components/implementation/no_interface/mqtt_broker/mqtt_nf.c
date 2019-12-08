#include <eos_pkt.h>
#include <nf_hypercall.h>
#include <arpa/inet.h>

//#include <lwip/init.h>
//#include <lwip/netif.h>
//#include <lwip/tcp.h>
//#include <lwip/stats.h>
//#include <lwip/prot/tcp.h>

#define ETHER_ADDR_LEN 6
#define EOS_MAX_CONNECTION 10
//#ifdef LEN_2
//#undef LEN_2
//#endif
#define LEN_2

static int conf_file_idx = 0;
static const u16_t port = 0x01BB; // 443
static u16_t tx_port; // 443
static struct ip4_addr ip, mask, gw;
static struct netif cos_if;
static vaddr_t shmem_addr;
static int idx;
static struct eos_ring *input_ring, *output_ring;
struct ether_addr eth_src, eth_dst;
uint16_t ether_type;
static int eth_copy = 0;
static int bump_alloc = 0;
unsigned long long udp_start, udp_end;

struct udp_hdr {
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t dgram_len;
	uint16_t dgram_cksum;
} __attribute__((__packed__));

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
	int id;
};

enum echoserver_state {
	ES_NONE = 0,
	ES_ACCEPTED,
	ES_RECEIVED,
	ES_CLOSING
};

static struct echoserver_struct echo_conn[EOS_MAX_CONNECTION];

static void
print_port(struct ether_hdr *dst) {
	int t = 0;
	for (t = 0; t < ETHER_ADDR_LEN; t++) {
		printc("%x__", dst->dst_addr.addr_bytes[t]);
	}
	printc("\n");
}

static void
ssl_print_pkt(void * pkt, int len) {
	int i = 0;
	printc("{ ");
	for (i = 0; i < len; i++) {
		printc("%02x ", ((uint8_t*)(pkt))[i]);
	}
	printc("}\n");
}

static inline void
ether_addr_copy(struct ether_addr *src, struct ether_addr *dst)
{
	*dst = *src;
}

static inline u16_t
ip_cksum(u16_t *ip, int len)
{
	long sum = 0;  /* assume 32 bit long, 16 bit short */

	while(len > 1) {
		sum += *( ip)++;
		if(sum & 0x80000000) {  /* if high order bit set, fold */
			sum = (sum & 0xFFFF) + (sum >> 16);
		}
		len -= 2;
	}

	if(len) {      /* take care of left over byte */
		sum += (u16_t) *(unsigned char *)ip;
	}

	while(sum>>16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	return ~sum;
}

/*static int
ssl_output(void *pl, int len)
{
	void *snd_pkt;
	int r;

	//pl      = p->payload;
	//len     = sizeof(struct ether_hdr) + p->len;
	snd_pkt = eos_pkt_allocate(input_ring, len);
	eth_hdr = (struct ether_hdr*)snd_pkt;

	// generate new ether_hdr
	//ether_addr_copy(&eth_src, &eth_hdr->src_addr);
	//ether_addr_copy(&eth_dst, &eth_hdr->dst_addr);
	//eth_hdr->ether_type = ether_type;

	memcpy((snd_pkt + sizeof(struct ether_hdr)), pl, p->len);
	r = eos_pkt_send(output_ring, snd_pkt, len, tx_port);
	assert(!r);
	return ERR_OK;
}*/

void *
udp_get_packet(int *len, u16_t *port, unsigned long long *deadline, unsigned long long *arrive)
{
	int err, r = 0;
	void *pkt;

	eos_pkt_collect(input_ring, output_ring);
	pkt = eos_pkt_recv_test(input_ring, len, port, deadline, arrive, &err, output_ring);
	//print_port((struct eth_hdr *)pkt);
	while (unlikely(!pkt)) {
		//printc("udp_get_packet: %d\n", err);
		if (err == -EBLOCK) {
			nf_hyp_block();
		}
		else if (err == -ECOLLET) eos_pkt_collect(input_ring, output_ring);
		pkt = eos_pkt_recv_test(input_ring, len, port, deadline, arrive, &err, output_ring);
	}
	/*if (unlikely(!eth_copy)) {
		struct ether_hdr *eth_hdr = (struct ether_hdr *)pkt;
		eth_copy = 1;
		ether_addr_copy(&eth_hdr->src_addr, &eth_dst);
		ether_addr_copy(&eth_hdr->dst_addr, &eth_src);
		ether_type = eth_hdr->ether_type;
	}*/
	return pkt;
}

int
udp_spin()
{
	unsigned long long start, end;
	int spin_time = 0;
	int ret = 0;

	start = ps_tsc();
	//printc("in udp application\n");
	switch (idx) {
	case 0:
	{
#ifdef LEN_2
		spin_time = 10;
		ret = -1;
#else
		spin_time = 25;
		ret = 0;
#endif
		break;
	}
	case 1:
	{
#ifdef LEN_2
		spin_time = 5;
		ret = -1;
#else
		assert(0);
		ret = -2;
#endif
		break;
	}
	case 2:
	{
#ifdef LEN_2
		spin_time = 10;
		ret = 0;
#else
		assert(0);
		ret = -2;
#endif
		break;
	}
	}
	end = ps_tsc();
	while ((end - start) < (unsigned long long)spin_time * (unsigned long long)2700) {
		__asm__ __volatile("rep;nop": : :"memory");
		end = ps_tsc();
	}

	return ret;
}

void
udp_process(void *pkt, int pkt_len, int *ret_len)
{
	struct ether_hdr *eth_hdr;
	struct ip4_hdr   *ip_hdr;
	struct udp_hdr   *udp;
	struct ether_addr eth_temp;
	uint32_t tmp;
	uint16_t tmp_udp;
	int ret = 0;

	eth_hdr = (struct ether_hdr *)pkt;
	ip_hdr  = (struct ip4_hdr *)(((char *)pkt) + sizeof(struct ether_hdr));
	udp     = (struct udp_hdr *)(((char *)ip_hdr) + sizeof(struct ip4_hdr));
	
	/* reform response */
	ret = udp_spin();
	if (ret) return;
	//if (ret != 0) assert(0);

	ether_addr_copy(&eth_hdr->dst_addr, &eth_temp);
	ether_addr_copy(&eth_hdr->src_addr, &eth_hdr->dst_addr);
	ether_addr_copy(&eth_temp, &eth_hdr->src_addr);

	tmp                  = ip_hdr->src_addr;
	ip_hdr->src_addr     = ip_hdr->dst_addr;
	ip_hdr->dst_addr     = tmp;

	tmp_udp              = udp->src_port;
	udp->src_port        = udp->dst_port;
	udp->dst_port        = tmp_udp;

	/* fix length */
	// len

	udp->dgram_cksum     = 0;
	ip_hdr->hdr_checksum = 0;
	ip_hdr->hdr_checksum = ip_cksum((u16_t *)ip_hdr, 20);

	ret_len = pkt_len;
	return;
}

static void
udp_server_run()
{
	int len, r, ret;
	void *pkt;
	unsigned long long res_us, deadline, arrive;
	
	while(1) {
		//printc("udp_server run\n");
		pkt = udp_get_packet(&len, &tx_port, &deadline, &arrive);
		assert(pkt);
		assert(len <= EOS_PKT_MAX_SZ);
		udp_start = ps_tsc();
		/*if (input_ring->cached.idx == 1) {
			output_ring->cached.cnt = EOS_PKT_PER_ENTRY + 1;
		}*/
		udp_process(pkt, len, &len);
		assert(len < EOS_PKT_MAX_SZ);
		ret = eos_pkt_send_test(output_ring, pkt, len, tx_port, deadline, arrive);
		/*if (input_ring->cached.idx == input_ring->cached.cnt) {
			//the end of this batch
			output_ring->cached.cnt = output_ring->cached.idx;
			ret = eos_pkt_send_flush_force(output_ring);
		}*/
		udp_end = ps_tsc();
		res_us = (udp_end-udp_start)/(unsigned long long)2700;
		//printc("WCET: %llu\n", res_us);
	}
}
/*
void
cos_init(void *args) 
{
	nf_hyp_confidx_get(&conf_file_idx);
	nf_hyp_get_shmem_addr(&shmem_addr);
	nf_hyp_nf_idx_get(&idx);

	input_ring  = get_input_ring((void *)shmem_addr);
	output_ring = get_output_ring((void *)shmem_addr);

	if (conf_file_idx == -1) {
		udp_server_run();
	} else {
		printc("udp server pre populate done\n");
		nf_hyp_checkpoint(cos_spd_id());
		udp_server_run();
	}
}*/
