#include <sl.h>
#include <sl_consts.h>
#include <sl_mod_policy.h>
#include <sl_plugins.h>

#define SL_FPRR_NPRIOS         32
#define SL_FPRR_PRIO_HIGHEST   0
#define SL_FPRR_PRIO_LOWEST    (SL_FPRR_NPRIOS-1)

#define SL_FPRR_PERIOD_US_MIN  SL_MIN_PERIOD_US

struct ps_list_head threads[NUM_CPU][SL_FPRR_NPRIOS] CACHE_ALIGNED;

/* No RR yet */
void
sl_mod_execution(struct sl_thd_policy *t, cycles_t cycles)
{ }

struct sl_thd_policy *
sl_mod_schedule(void)
{
	int i, cpu = cos_cpuid();
	struct sl_thd_policy *t;

	for (i = 0 ; i < SL_FPRR_NPRIOS ; i++) {
		if (ps_list_head_empty(&threads[cpu][i])) continue;
		t = ps_list_head_first_d(&threads[cpu][i], struct sl_thd_policy);

		/*
		 * We want to move the selected thread to the back of the list.
		 * Otherwise fprr won't be truly round robin
		 */
		ps_list_rem_d(t);
		ps_list_head_append_d(&threads[cpu][i], t);

		return t;
	}

	return NULL;
}

void
sl_mod_block(struct sl_thd_policy *t)
{
	ps_list_rem_d(t);
}

void
sl_mod_wakeup(struct sl_thd_policy *t)
{
	assert(t->priority <= SL_FPRR_PRIO_LOWEST);

	ps_list_rem_d(t);
	ps_list_head_add_d(&threads[cos_cpuid()][t->priority], t);
}

void
sl_mod_yield(struct sl_thd_policy *t, struct sl_thd_policy *yield_to)
{
	assert(t->priority <= SL_FPRR_PRIO_LOWEST);

	ps_list_rem_d(t);
	ps_list_head_append_d(&threads[cos_cpuid()][t->priority], t);
}

void
sl_mod_thd_create(struct sl_thd_policy *t)
{
	t->priority    = SL_FPRR_PRIO_LOWEST;
	t->period      = 0;
	t->period_usec = 0;
	ps_list_init_d(t);
}

void
sl_mod_thd_delete(struct sl_thd_policy *t)
{ ps_list_rem_d(t); }

void
sl_mod_thd_param_set(struct sl_thd_policy *t, sched_param_type_t type, unsigned int v)
{
	int cpu = cos_cpuid();

	switch (type) {
	case SCHEDP_PRIO:
	{
		assert(v < SL_FPRR_NPRIOS);
		t->priority = v;
		sl_thd_setprio(sl_mod_thd_get(t), t->priority);
		ps_list_rem_d(t); 	/* if we're already on a list, and we're updating priority */
		ps_list_head_append_d(&threads[cpu][v], t);
		break;
	}
	case SCHEDP_PRIO_BLOCK:
	{
		assert(v < SL_FPRR_NPRIOS);
		t->priority = v;
		sl_thd_setprio(sl_mod_thd_get(t), t->priority);
		sl_mod_thd_get(t)->state = SL_THD_BLOCKED;
		break;
	}
	case SCHEDP_WINDOW:
	{
		assert(v >= SL_FPRR_PERIOD_US_MIN);
		t->period_usec    = v;
		t->period         = sl_usec2cyc(v);
		/* FIXME: synchronize periods for all tasks */

		break;
	}
	case SCHEDP_BUDGET:
	{
		break;
	}
	default: assert(0);
	}
}

void
sl_mod_init(void)
{
	int i, cpu = cos_cpuid();

	memset(threads[cpu], 0, sizeof(struct ps_list_head) * SL_FPRR_NPRIOS);
	for (i = 0 ; i < SL_FPRR_NPRIOS ; i++) {
		ps_list_head_init(&threads[cpu][i]);
	}
}
