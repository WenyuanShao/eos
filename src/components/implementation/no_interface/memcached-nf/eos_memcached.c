#include <eos_pkt.h>
#include <nf_hypercall.h>
#include "memcached.h"

#define MC_COLLECT_THRESH (32*EOS_PKT_PER_ENTRY)

extern int mc_process(void *pkt, int pkt_len, int *ret_len);
extern int mc_populate(int num_key);
static int prev_collect = MC_COLLECT_THRESH, conf_file_idx = 0;
static vaddr_t shmem_addr;
static struct eos_ring *input_ring, *ouput_ring;

static int
mc_pkt_collect(struct eos_ring *recv, struct eos_ring *sent)
{
	if (!(prev_collect--)) {
		prev_collect = eos_pkt_collect(recv, sent)*EOS_PKT_PER_ENTRY;
		if (!prev_collect) nf_hyp_block();
		else prev_collect--;
	}
	return prev_collect;
}

void *
eos_get_packet(int *len, int *port)
{
       int err, c = 0;
       void *pkt;

       mc_pkt_collect(input_ring, ouput_ring);
       pkt = eos_pkt_recv(input_ring, len, port, &err);
       while (unlikely(!pkt)) {
	       if (err == -EBLOCK) {
		       eos_pkt_send_flush(ouput_ring);
		       nf_hyp_block();
	       } else if (err == -ECOLLET) mc_pkt_collect(input_ring, ouput_ring);
	       pkt = eos_pkt_recv(input_ring, len, port, &err);
       }

      
       return pkt;
}

static void
mc_server_run()
{
       void *pkt;
       int len, port, r;

       while (1) {
	       pkt = eos_get_packet(&len, &port);
	       assert(pkt);
	       assert(len <= EOS_PKT_MAX_SZ);
	       /* printc("dbg l %d \n", len); */
	       mc_process(pkt, len, &len);
	       assert(len < EOS_PKT_MAX_SZ);
	       /* printc("dbg n %d\n", len); */
	       r = eos_pkt_send(ouput_ring, pkt, len, port);
	       assert(!r);
       }
}

void
cos_init(void *args)
{
       nf_hyp_confidx_get(&conf_file_idx);
       nf_hyp_get_shmem_addr(&shmem_addr); 
       printc ("dbg shmem %p conf %d\n", shmem_addr, conf_file_idx);
       input_ring = get_input_ring((void *)shmem_addr);
       ouput_ring = get_output_ring((void *)shmem_addr);

       if (conf_file_idx == -1) {
	       mc_server_run();
       } else {
	       hash_init(JENKINS_HASH);
	       assoc_init(0);
	       item_init();
	       /* printc("dbg mc 2\n"); */
	       mc_populate(10);
	       nf_hyp_checkpoint(cos_spd_id());
	       mc_server_run();
       }
}

