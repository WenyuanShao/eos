#ifndef __ESO_RING_H__
#define __ESO_RING_H__

#include <cos_debug.h>
#include <consts.h>
#include <cos_types.h>

#define EOS_PKT_MAX_SZ 256 /*the same as Click*/
#define EOS_RING_SIZE 128 /* 256 */
#define EOS_PKT_PER_ENTRY 8 /* 8 */
#define EOS_PKT_COLLECT_MULTIP 1 /* EOS_PKT_PER_ENTRY */
#define EOS_RING_MASK (EOS_RING_SIZE - 1)
#define RING_NODE_PAD_SZ (2*CACHE_LINE - 3*sizeof(int) - EOS_PKT_PER_ENTRY*sizeof(struct pkt_meta))
#define GET_RING_NODE(r, h) ((volatile struct eos_ring_node *)(&((r)->ring[(h)])))

typedef enum {
	PKT_EMPTY = 0,
	PKT_FREE,
	PKT_RECV_READY,
	PKT_RECV_DONE,
	PKT_SENT_READY,
	PKT_SENT_DONE,
	PKT_TXING,
} pkt_states_t;

struct pkt_meta {
	void *pkt;
	int pkt_len, port;
}__attribute__((packed));

struct eos_ring;

struct eos_ring_node {
	int cnt, idx;
	struct pkt_meta pkts[EOS_PKT_PER_ENTRY];
	pkt_states_t state;
	char pad[RING_NODE_PAD_SZ];
	/* struct eos_ring *_in, *_out; */
} __attribute__((packed));

struct eos_ring {
	struct eos_ring_node *ring;  /* read only */
	void *ring_phy_addr;
	int coreid, thdid;
	char pad1[2 * CACHE_LINE - 4*sizeof(struct eos_ring_node *)];
	int free_head, pkt_cnt;        /* shared head */
	char pad2[2 * CACHE_LINE - 2*sizeof(int)];
	int mca_head;        /* mca access only */
	char pad3[2 * CACHE_LINE - sizeof(int)];
	int head, tail;    /* nf access only */
	char pad4[2 * CACHE_LINE - 2*sizeof(int)];
	struct eos_ring_node cached;
} __attribute__((packed));

extern void *cos_map_virt_to_phys(void *addr);

/**
 * shared ring buffer and packet memory layout:    
 *
 * +-----------------------------------------------------------------------+-------PKT MEM SZ----+ 
 * | input ring | EOS_RING_SIZE nodes | output ring | EOS_RING_SIZE nodes  | packet |...| packet |
 * +-----------------------------------------------------------------------+---------------------+ 
 *
 */
static inline struct eos_ring *
get_input_ring(void *rh){
	return (struct eos_ring *)rh;
}

static inline struct eos_ring *
get_output_ring(void *rh){
	struct eos_ring *output_ring = (struct eos_ring *)((char *)rh + sizeof(struct eos_ring) + 
							   EOS_RING_SIZE * sizeof(struct eos_ring_node));
	return output_ring;
}

/*We need both addresses in order to corectly page align*/
static inline void
eos_rings_init(void *rh)
{
	struct eos_ring *input_ring, *output_ring;
	char *pkts, *end_of_rings;
	int i, j;

	assert(((unsigned long)rh & (~PAGE_MASK)) == 0);

	input_ring = get_input_ring(rh);
	memset(input_ring, 0, sizeof(struct eos_ring));
	input_ring->ring = (struct eos_ring_node *)((char *)input_ring + sizeof(struct eos_ring));
	input_ring->ring_phy_addr = cos_map_virt_to_phys(rh);

	output_ring = get_output_ring(rh);
	memset(output_ring, 0, sizeof(struct eos_ring));
	output_ring->ring = (struct eos_ring_node *)((char *)output_ring + sizeof(struct eos_ring));
	output_ring->ring_phy_addr = input_ring->ring_phy_addr + ((char *)output_ring - (char *)rh);
	output_ring->cached.state = PKT_SENT_READY;

	end_of_rings = (char *)output_ring->ring + EOS_RING_SIZE * sizeof(struct eos_ring_node);
	pkts = (char *)round_up_to_page(end_of_rings);

	for(i=0; i<EOS_RING_SIZE; i++) {
		memset(output_ring->ring + i, 0, sizeof(struct eos_ring_node));

		memset(input_ring->ring + i, 0, sizeof(struct eos_ring_node));
		input_ring->ring[i].state = PKT_FREE;
		for(j=0; j<EOS_PKT_PER_ENTRY; j++) {
			input_ring->ring[i].pkts[j].pkt_len = EOS_PKT_MAX_SZ;
			input_ring->ring[i].pkts[j].pkt     = pkts;
			pkts                               += EOS_PKT_MAX_SZ;
		}
	}
}

#endif /* __ESO_RING_H__ */

