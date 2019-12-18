#include <eos_pkt.h>
#include <nf_hypercall.h>
#include <arpa/inet.h>

#define ETHER_ADDR_LEN 6
#define LEN_2

static int conf_file_idx = 0;
static u16_t tx_port; // 443
static vaddr_t shmem_addr;
static int idx;
static struct eos_ring *input_ring, *output_ring;
struct ether_addr eth_src, eth_dst;
uint16_t ether_type;
int global_flush;

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

struct mqtt_buffer {
	unsigned long long deadline;
	unsigned long long arrive;
	int raw_len, len;
	void *raw_pkt;
	void *pkt;
};

struct mqtt_buffer mqtt_buffer;
#define MQTT_HDR_LEN (sizeof(struct ether_hdr) + sizeof(struct ip4_hdr) + sizeof(struct udp_hdr))
char stored_hdr[MQTT_HDR_LEN];

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

static inline void mqtt_ntop(void *addrptr, char *strptr, size_t len)
{
	char *p = (char *)addrptr;

	char temp[16];
	sprintf(temp, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);

	assert(len >= 16);

	memcpy(strptr, temp, sizeof(temp));
}

char *
mqtt_dummy_recvfrom(int sock, char *msg, int buffer_len, int flag, int *len)
{
	static char     ret[20];
	struct ip4_hdr *ip_hdr;
	struct udp_hdr *udp_hdr;

	ip_hdr = (struct ip4_hdr *)((char *)mqtt_buffer.raw_pkt + sizeof(struct ether_hdr));
	udp_hdr = (struct udp_hdr *)((char *)ip_hdr + sizeof(struct ip4_hdr));

	assert(buffer_len >= mqtt_buffer.len);
	memcpy((void *)msg, mqtt_buffer.pkt, mqtt_buffer.len);
	*len = mqtt_buffer.len;
	mqtt_ntop(&(ip_hdr->src_addr), ret, 16);
	sprintf(&ret[9], ":%ld", ntohs(udp_hdr->src_port));
	
	return ret;
}

int
mqtt_dummy_sendto(int sock, char *pkt_buf, int len, int flag, int port, uint32_t *dst_addr)
{
	void             *raw_pkt = mqtt_buffer.raw_pkt;
	void             *pkt;
	void             *snd_pkt;
	struct ip4_hdr   *ip_hdr;
	struct udp_hdr   *udp;
	int               ret, tot_len;

	tot_len = mqtt_buffer.raw_len - mqtt_buffer.len + len;
	snd_pkt = eos_pkt_allocate(input_ring, tot_len);
	memcpy(snd_pkt, stored_hdr, MQTT_HDR_LEN);

	ip_hdr  = (struct ip4_hdr *)((char *)snd_pkt + sizeof(struct ether_hdr));
	udp     = (struct udp_hdr *)((char *)ip_hdr + sizeof(struct ip4_hdr));
	pkt     = (void *)((char *)udp + sizeof(struct udp_hdr));

	ip_hdr->total_len    = htons(len + sizeof(struct udp_hdr) + sizeof(struct ip4_hdr));
	ip_hdr->hdr_checksum = 0;
	ip_hdr->hdr_checksum = ip_cksum((u16_t *)ip_hdr, 20);
	ip_hdr->dst_addr     = *dst_addr;

	udp->dst_port        = port;
	udp->dgram_len       = htons(len + sizeof(struct udp_hdr));

	memcpy(pkt, pkt_buf, len);
	ret = eos_pkt_send_test(output_ring, snd_pkt, tot_len, tx_port, mqtt_buffer.deadline, mqtt_buffer.arrive);

	return 1;
}

void *
mqtt_udp_process(void *raw_pkt, int pkt_len, int *ret_len)
{
	struct ether_hdr *eth_hdr;
	struct ip4_hdr   *ip_hdr;
	struct udp_hdr   *udp;
	struct ether_addr eth_temp;
	uint32_t tmp;
	uint16_t tmp_udp;
	int ret = 0;
	void *pkt;

	memcpy(stored_hdr, raw_pkt, MQTT_HDR_LEN);
	eth_hdr = (struct ether_hdr *)stored_hdr;
	ip_hdr  = (struct ip4_hdr *)(stored_hdr + sizeof(struct ether_hdr));
	udp     = (struct udp_hdr *)(((char *)ip_hdr) + sizeof(struct ip4_hdr));
	
	ether_addr_copy(&eth_hdr->dst_addr, &eth_temp);
	ether_addr_copy(&eth_hdr->src_addr, &eth_hdr->dst_addr);
	ether_addr_copy(&eth_temp, &eth_hdr->src_addr);

	tmp                  = ip_hdr->src_addr;
	ip_hdr->src_addr     = ip_hdr->dst_addr;
	ip_hdr->dst_addr     = tmp;

	tmp_udp              = udp->src_port;
	udp->src_port        = udp->dst_port;
	udp->dst_port        = tmp_udp;

	udp->dgram_cksum     = 0;

	*ret_len = pkt_len - MQTT_HDR_LEN;

	pkt = (void *)((char *)raw_pkt + MQTT_HDR_LEN);

	return pkt;
}

int
mqtt_dummy_select(void)
{
	int err, r = 0;
	void *raw_pkt, *pkt;
	unsigned long long deadline, arrive;
	int len, raw_len, ret;

	if (global_flush) {
		output_ring->cached.cnt = output_ring->cached.idx;
		ret = eos_pkt_send_flush_force(output_ring);
	}
	eos_pkt_collect(input_ring, output_ring);
	raw_pkt = eos_pkt_recv_test(input_ring, &raw_len, &tx_port, &deadline, &arrive, &err, output_ring);
	int test =  3*sizeof(short) + 2*sizeof(unsigned long long) + sizeof(pkt_states_t) + EOS_PKT_PER_ENTRY*sizeof(struct pkt_meta) - 2*CACHE_LINE;
	while (unlikely(!raw_pkt)) {
		if (err == -EBLOCK) {
			nf_hyp_block();
		}
		else if (err == -ECOLLET) eos_pkt_collect(input_ring, output_ring);
		raw_pkt = eos_pkt_recv_test(input_ring, &raw_len, &tx_port, &deadline, &arrive, &err, output_ring);
	}

	if (input_ring->cached.idx == 1) {
		output_ring->cached.cnt = EOS_PKT_PER_ENTRY + 1;
	}

	if (input_ring->cached.cnt == input_ring->cached.idx)
		global_flush = 1;
	else
		global_flush = 0;

	mqtt_buffer.raw_len      = raw_len;
	mqtt_buffer.raw_pkt      = raw_pkt;
	mqtt_buffer.deadline     = deadline;
	mqtt_buffer.arrive       = arrive;
	pkt = mqtt_udp_process(raw_pkt, raw_len, &len);
	mqtt_buffer.pkt          = pkt;
	mqtt_buffer.len          = len;
	return 1;
}

void
mqtt_just_send()
{
	int len, r, ret, err;
	void *pkt;
	unsigned long long deadline, arrive;

	while(1) {

		/*if(global_flush) {
			output_ring->cached.cnt = output_ring->cached.idx;
			ret = eos_pkt_send_flush_force(output_ring);
		}*/
		eos_pkt_collect(input_ring, output_ring);
		//output_ring->cached.cnt = input_ring->cached.cnt;
		pkt = eos_pkt_recv_test(input_ring, &len, &tx_port, &deadline, &arrive, &err, output_ring);
		while (unlikely(!pkt)) {
			if (err == -EBLOCK) {
				nf_hyp_block();
			} else if (err == -ECOLLET) eos_pkt_collect(input_ring, output_ring);
			pkt = eos_pkt_recv_test(input_ring, &len, &tx_port, &deadline, &arrive, &err, output_ring);
		}
		assert(pkt);
		assert(len <= EOS_PKT_MAX_SZ);
		/*if (input_ring->cached.idx == 1) {
			output_ring->cached.cnt = EOS_PKT_PER_ENTRY + 1;
		}
		if (input_ring->cached.cnt = input_ring->cached.idx)
			global_flush = 1;
		else
			global_flush = 0;*/

		ret = eos_pkt_send_test(output_ring, pkt, len, tx_port, deadline, arrive);
	}
}

extern int mqtts_broker(void);

void
cos_init(void *args) 
{
	unsigned long long sp;
	nf_hyp_confidx_get(&conf_file_idx);
	nf_hyp_get_shmem_addr(&shmem_addr);
	nf_hyp_nf_idx_get(&idx);

	assert(idx < 3);

	input_ring  = get_input_ring((void *)shmem_addr);
	output_ring = get_output_ring((void *)shmem_addr);

	global_flush = 0;
	if (conf_file_idx == -1) {
		if (idx == 1) {
			mqtts_broker();
		} else {
			mqtt_just_send();
		}
	} else {
		printc("udp server pre populate done\n");
		nf_hyp_checkpoint(cos_spd_id());
		mqtts_broker();
	}
}
