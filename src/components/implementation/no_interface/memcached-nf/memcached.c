#include <arpa/inet.h>
#include <memcached.h>
#include <eos_pkt.h>

typedef unsigned short u16;
typedef unsigned long u32;
#define ETHER_ADDR_LEN  6
static int udp_len;
static char g_key[KEY_LENGTH], g_val[V_LENGTH];
static char format[20];

// UDP header info from: https://github.com/memcached/memcached/blob/master/doc/protocol.txt#L51
// The frame header is 8 bytes long, as follows (all values are 16-bit integers
// in network byte order, high byte first):
// 0-1 Request ID
// 2-3 Sequence number
// 4-5 Total number of datagrams in this message
// 6-7 Reserved for future use; must be 0
struct mc_udp_hdr {
    uint16_t req_id;
    uint16_t seq_num;
    uint16_t tot_dgrams;
    uint16_t extra;
} __attribute__((__packed__));

struct udp_hdr {
        uint16_t src_port;    
        uint16_t dst_port;    
        uint16_t dgram_len;   
        uint16_t dgram_cksum; 
} __attribute__((__packed__));

struct ipv4_hdr {
        uint8_t  version_ihl;           
        uint8_t  type_of_service;       
        uint16_t total_length;
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
        struct ether_addr d_addr; 
        struct ether_addr s_addr; 
        uint16_t ether_type;      
} __attribute__((__packed__));

static inline void
ether_addr_copy(struct ether_addr *ea_from, struct ether_addr *ea_to)
{
        /*
         * Use the common way, because of a strange gcc warning.
         */
        *ea_to = *ea_from;
}

static inline u16 
ip_cksum(u16 *ip, int len)
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
	        sum += (u16) *(unsigned char *)ip;
        }
          
        while(sum>>16) {
	        sum = (sum & 0xFFFF) + (sum >> 16);
        }

        return ~sum;
}

int
mc_set_key(char *key, int nkey, char *data, int nbytes)
{
	item *old_it = NULL, *it = NULL;
	unsigned int hv;

	int node = 0;
	hv = hash(key, nkey);
	it = do_item_alloc(key, nkey, 0, 0, nbytes+2, 0);
	if (!it) {
		printc("ERROR: item_alloc failed once?\n");
		return -1;
	}
	memcpy(ITEM_data(it), data, nbytes);
	it->hv = hv;

	old_it = do_item_get(key, nkey, hv);
	if (!old_it) do_item_link(it, hv);
	else do_item_replace(old_it, it, hv);
	return 0;
}

char *
mc_get_key(char *key, int nkey, int *nbyte)
{
	item *it;
	uint32_t hv;

	hv   = hash(key, nkey);
	it = do_item_get(key, nkey, hv);
	assert(it);

	if (it) {
		*nbyte = it->nbytes-2;
		return ITEM_data(it);
	} else {
		*nbyte = 0;
		return NULL;
	}
}

static void
reform_get_response(char *pkt_data, char *key, int key_len)
{
	int inc, byte = 0;
	char kk[20], *ret_v = NULL;
	
	inc = strlen("VALUE ");
	strncpy(pkt_data, "VALUE ", inc);
	pkt_data += inc;
	udp_len += inc;

	strncpy(pkt_data, key, key_len);
	pkt_data += key_len;
	udp_len += key_len;

	// flag = 0, byte length = 100 and cas = 0
	inc = sprintf(kk, " 0 %d\r\n", V_LENGTH);
	strncpy(pkt_data, kk, inc);
	pkt_data += inc;
	udp_len += inc;

	ret_v = mc_get_key(key, key_len, &byte);
	assert(byte == V_LENGTH);
	assert(ret_v);
	/* printc("dbg reponse %s\n", ret_v); */
	memcpy(pkt_data, ret_v, byte);
	pkt_data += byte;
	udp_len  += byte;
	
	inc = strlen("\r\nEND\r\n");
	strncpy(pkt_data, "\r\nEND\r\n", inc);
	udp_len += inc;
}

static void
reform_set_response(char *pkt_data, char *key, int key_len)
{
        char *p;
        int vl, inc;

        p = pkt_data + strlen("set ") + key_len + 1;
        sscanf(p, format, &vl, g_val);
	assert(vl == V_LENGTH);
        /* printc("dbg vl %d %s\n", vl, g_val); */
	mc_set_key(key, key_len, g_val, vl);

        inc = strlen("STORED\r\n");
        strncpy(pkt_data, "STORED\r\n", inc);
        pkt_data += inc;
        udp_len += inc;
}

static int
parse_mc_pkt(void *pkt, char** pkt_data_p, char *ret_key, int *keylen_p)
{
    	int key_len = 0;
    	struct mc_udp_hdr *udp_hdr = (struct mc_udp_hdr*) (pkt + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr));
	char* ascii_req = (char *)(udp_hdr) + sizeof(struct mc_udp_hdr);

	if(ascii_req[0] == 'V' || ascii_req[0] == 'v') return -1;
    
	*pkt_data_p = ascii_req;

	/* Step through request until next space (ignore first 4 characters 'GET/SET '*/
	for(key_len=0; key_len <= KEY_LENGTH; key_len++) {
		if(ascii_req[key_len+4] == ' ' || ascii_req[key_len+4] == '\r') {
			break;
		}
	}
	strncpy(ret_key, ascii_req + 4, key_len);
	*keylen_p = key_len;

	return 0;
}

int
mc_process(void *pkt, int pkt_len, int *ret_len) {
	char *pkt_data = NULL;
	int key_len;
	int old_udp_len;
	struct ether_hdr *eth_hdr;
	struct ipv4_hdr *ip_hdr;
	struct udp_hdr *udp;
	uint32_t tmp;
	uint16_t tmp_udp;
	struct ether_addr eth_temp;

	parse_mc_pkt(pkt, &pkt_data, g_key, &key_len);
	assert(pkt_data);
	assert(key_len == KEY_LENGTH);

	eth_hdr     = (struct ether_hdr *)pkt;
	ip_hdr      = (struct ipv4_hdr *) (pkt + sizeof(struct ether_hdr));
	udp         = (struct udp_hdr *)((char *)ip_hdr + sizeof(struct ipv4_hdr));
	old_udp_len = ntohs(udp->dgram_len);
	udp_len     = 16;
	if(pkt_data[0] == 'g') {
		/* reform response modifies udp_len */
		reform_get_response(pkt_data, g_key, key_len);
	} else {
		assert(pkt_data[0] == 's');
		reform_set_response(pkt_data, g_key, key_len);
	}
		
	/* switch headers */
	ether_addr_copy(&eth_hdr->d_addr, &eth_temp);
	ether_addr_copy(&eth_hdr->s_addr, &eth_hdr->d_addr);
	ether_addr_copy(&eth_temp, &eth_hdr->s_addr);

	tmp                  = ip_hdr->src_addr;
	ip_hdr->src_addr     = ip_hdr->dst_addr;
	ip_hdr->dst_addr     = tmp;

	tmp_udp              = udp->src_port;
	udp->src_port        = udp->dst_port;
	udp->dst_port        = tmp_udp;

	/* fix lengths */
	udp->dgram_len       = htons(udp_len);
	ip_hdr->total_length = htons(20 + udp_len);

	udp->dgram_cksum     = 0;
	ip_hdr->hdr_checksum = 0;
	ip_hdr->hdr_checksum = ip_cksum((u16 *)ip_hdr, 20);

	*ret_len             = pkt_len + (udp_len - old_udp_len);

        return 0;
}

int
mc_populate(int num_key)
{
	int i;
	char *str;

	sprintf(format, "%%*d %%*d %%d %%%ds", V_LENGTH);
	memset(g_val, '$', V_LENGTH);
	for(i=0; i<KEY_PREFIX_LENGTH; i++) g_key[i] = 'k';
	g_key[i++] = '-';
	str = &(g_key[i]);
	for(i=0; i<num_key; i++) {
		sprintf(str, "%06d", i);
		mc_set_key(g_key, KEY_LENGTH, g_val, V_LENGTH);
	}
	return 0;
}
