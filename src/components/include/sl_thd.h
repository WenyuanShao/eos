/**
 * Redistribution of this file is permitted under the BSD two clause license.
 *
 * Copyright 2017, The George Washington University
 * Author: Gabriel Parmer, gparmer@gwu.edu
 */

#ifndef SL_THD_H
#define SL_THD_H

#include <ps.h>
#include <cos_debug.h>

#define SL_THD_EVENT_LIST event_list

typedef enum {
	SL_THD_FREE = 0,
	SL_THD_BLOCKED,
	SL_THD_BLOCKED_TIMEOUT,
	SL_THD_WOKEN, /* Unused because kernel may send redundant scheduling events! if a race causes a wakeup before the inevitable block */
	SL_THD_RUNNABLE,
	SL_THD_DYING,
} sl_thd_state_t;

typedef enum {
	SL_THD_PROPERTY_OWN_TCAP = 1,      /* Thread owns a tcap */
	SL_THD_PROPERTY_SEND     = (1<<1), /* use asnd to dispatch to this thread */
} sl_thd_property_t;

struct event_info {
	int         blocked; /* 1 - blocked. 0 - awoken */
	cycles_t    cycles;
	tcap_time_t timeout;
};

struct sl_thd {
	sl_thd_state_t       state;
	sl_thd_property_t    properties;
	struct cos_aep_info *aepinfo;
	asndcap_t            sndcap;
	struct sl_thd       *dependency;
	struct sl_thd       *schedthd;

	/* TODO: unionize! lot of unused members for a child thd */
	cbuf_t              shmid;
	struct ck_ring      *ch_ring;
	struct sl_child_notification *ch_ringbuf;

	tcap_prio_t prio;
	tcap_res_t  budget;        /* budget if this thread has it's own tcap */
	cycles_t    last_replenish;
	cycles_t    period;
	cycles_t    periodic_cycs; /* for implicit periodic timeouts */
	cycles_t    timeout_cycs;  /* next timeout - used in timeout API */
	cycles_t    wakeup_cycs;   /* actual last wakeup - used in timeout API for jitter information, etc */
	int         timeout_idx;   /* timeout heap index, used in timeout API */

	struct event_info event_info;
	struct ps_list    SL_THD_EVENT_LIST; /* list of events for the scheduler end-point */
};

static inline struct cos_aep_info *
sl_thd_aepinfo(struct sl_thd *t)
{ return (t->aepinfo); }

static inline thdcap_t
sl_thd_thdcap(struct sl_thd *t)
{ return sl_thd_aepinfo(t)->thd; }

static inline tcap_t
sl_thd_tcap(struct sl_thd *t)
{ return sl_thd_aepinfo(t)->tc; }

static inline arcvcap_t
sl_thd_rcvcap(struct sl_thd *t)
{ return sl_thd_aepinfo(t)->rcv; }

static inline asndcap_t
sl_thd_asndcap(struct sl_thd *t)
{ return t->sndcap; }

static inline thdid_t
sl_thd_thdid(struct sl_thd *t)
{ return sl_thd_aepinfo(t)->tid; }

#endif /* SL_THD_H */
