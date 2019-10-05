#include <eos_pkt.h>
#include <nf_hypercall.h>

#define ETHER_ADDR_LEN 6

static int conf_file_idx = 0;
static int idx;
static vaddr_t shmem_addr;
static struct eos_ring *input_ring, *output_ring;
static unsigned long long start, end;

struct ether_addr {
	u8_t addr_bytes[ETHER_ADDR_LEN];
} __attribute__((__packed__));

struct ether_hdr {
	struct ether_addr dst_addr;
	struct ether_addr src_addr;
	u16_t ether_type;
} __attribute__((__packed__));

static inline void
ether_addr_switch(struct ether_addr *src, struct ether_addr *dst)
{
	struct ether_addr *tmp;

	*tmp = *dst;
	*dst = *src;
	*src = *tmp;
}


static void
eos_dummy_process(void *pkt)
{
	int spin_time = 0;
	struct ether_hdr *eth_hdr;

	start = end = ps_tsc();

	eth_hdr = (struct ether_hdr *)pkt;
	ether_addr_switch(&eth_hdr->src_addr, &eth_hdr->dst_addr);

	switch (idx){
	case 0:
	{
		spin_time = 10;
		break;
	}
	case 1:
	{
		spin_time = 20;
		break;
	}
	case 2:
	{
		spin_time = 30;
		break;
	}
	}
	while((end - start) < (unsigned long long)spin_time * (unsigned long long)2700) {
		__asm__ __volatile__("rep;nop": : :"memory");
		end = ps_tsc();
	}
}

static void *
eos_get_packet(int *len, int *port)
{
	int err = 0;
	void *pkt;

	eos_pkt_collect(input_ring, output_ring);
	pkt = eos_pkt_recv(input_ring, len, port, &err, output_ring);
	while (unlikely(!pkt)) {
		if (err == -EBLOCK) nf_hyp_block();
		else if (err == -ECOLLET) eos_pkt_recv(input_ring, len, port, &err, output_ring);
	}

	return pkt;
}

static void
eos_dummy_nf_run()
{
	void *pkt;
	int len, port, r;

	while (1) {
		pkt = eos_get_packet(&len, &port);
		assert(pkt);
		assert(len <= EOS_PKT_MAX_SZ);
		eos_dummy_process(pkt);
		r = eos_pkt_send(output_ring, pkt, len, port);
		assert(!r);
	}
}

void
cos_init(void *args)
{
	nf_hyp_confidx_get(&conf_file_idx);
	nf_hyp_get_shmem_addr(&shmem_addr);
	nf_hyp_nf_idx_get(&idx);

	input_ring = get_input_ring((void *)shmem_addr);
	output_ring = get_input_ring((void *)shmem_addr);

	if (conf_file_idx == -1) {
		eos_dummy_nf_run();
	} else {
		printc("dummy nf pre populate done\n");
		nf_hyp_checkpoint(cos_spd_id());
		eos_dummy_nf_run();
	}
}
