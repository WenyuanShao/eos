#include <stdlib.h>

void *global_click_pkt = NULL;

void *
click_pkt_alloc()
{
	return global_click_pkt;
}

void
click_pkt_free(void *packet)
{
	global_click_pkt = packet;
}
