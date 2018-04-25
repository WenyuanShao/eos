#include <cobj_format.h>
#include <sl.h>

#include "fwp_manager.h"
#include "fwp_chain_cache.h"
#include "eos_ring.h"
#include "eos_utils.h"
#include "eos_mca.h"
#include "ninf.h"

/* Assembly function for sinv from new component */
extern word_t nf_entry_rets_inv(invtoken_t cur, int op, word_t arg1, word_t arg2, word_t *ret2, word_t *ret3);

/* 
 * IDs one and two are given to the booter 
 * component and the first loaded component
 */
long next_nfid = 2, next_chain_id = 0, next_shmem_id = 0, next_template_id = 0;

vaddr_t shmem_addr;

struct mem_seg *t_seg;
struct mem_seg *d_seg;
unsigned long cinfo_offset;
vaddr_t s_addr, shmem_inv_addr;
struct sl_thd  *mca_thd;
struct mem_seg templates[EOS_MAX_NF_TYPE_NUM];
static struct nf_chain chains[EOS_MAX_CHAIN_NUM];
struct click_info chld_infos[EOS_MAX_NF_NUM];

static void
_alloc_tls(struct cos_compinfo *parent_cinfo_l, struct cos_compinfo *chld_cinfo_l, thdcap_t tc, size_t tls_size)
{
    vaddr_t tls_seg, addr, dst_pg, dst_seg;
    
    tls_seg = (vaddr_t) cos_page_bump_allocn(parent_cinfo_l, tls_size);
    assert(tls_seg);
    dst_seg = cos_mem_alias(chld_cinfo_l, parent_cinfo_l, tls_seg);
    assert(dst_seg);
    for (addr = PAGE_SIZE; addr < tls_size; addr += PAGE_SIZE) {
        dst_pg = cos_mem_alias(chld_cinfo_l, parent_cinfo_l, (tls_seg + addr));
        assert(dst_pg);
    }
    
    cos_thd_mod(parent_cinfo_l, tc, (void *)dst_seg); 
}

static int
fwp_ci_get(struct cobj_header *h, vaddr_t *comp_info)
{
       int i = 0;

       for (i = 0; i < (int)h->nsymb; i++) {
              struct cobj_symb *symb;

              symb = cobj_symb_get(h, i);
              assert(symb);

              if (symb->type == COBJ_SYMB_COMP_INFO) {
                     *comp_info = symb->vaddr;
                     return 0;
              }

       }
       return 1;
}


/*
 * COS internals for creating a new component
 */
static void
_fwp_fork(struct cos_compinfo *parent_cinfo_l, struct click_info *fork_info, 
              struct mem_seg *text_seg, struct mem_seg *data_seg, 
              struct mem_seg *ring_seg, int conf_file_idx, vaddr_t start_addr)
{
       struct cos_aep_info *fork_aep = cos_sched_aep_get(&fork_info->def_cinfo);
       struct cos_compinfo *fork_cinfo = cos_compinfo_get(&fork_info->def_cinfo);
       pgtblcap_t ckpt;
       captblcap_t ckct;
       vaddr_t dest, addr, heap_ptr;
       unsigned long size;

       //printc("forking new click component\n");
       ckct = cos_captbl_alloc(parent_cinfo_l);
       assert(ckct);

       ckpt = cos_pgtbl_alloc(parent_cinfo_l);
       assert(ckpt);

       heap_ptr = round_up_to_pgd_page(shmem_addr + FWP_MAX_MEMSEGS * FWP_MEMSEG_SIZE);
       cos_compinfo_init(fork_cinfo, ckpt, ckct, 0, heap_ptr,
                            BOOT_CAPTBL_FREE, parent_cinfo_l);

       size = heap_ptr - start_addr;
       printc("size: %p - %p\n", heap_ptr, start_addr);
       if (!cos_pgtbl_intern_alloc(parent_cinfo_l, ckpt, start_addr, size)) BUG();
       for (dest = 0; dest < ring_seg->size; dest += PAGE_SIZE) {
              cos_mem_alias_at(fork_cinfo, (ring_seg->addr + dest), parent_cinfo_l, (ring_seg->addr + dest));
       }

       /* initialize rings*/
       eos_rings_init((void *)ring_seg->addr);

       fork_info->conf_file_idx = conf_file_idx;
       fork_info->nf_id = next_nfid;
}

static vaddr_t
_alias_click(struct cos_compinfo *parent_cinfo, struct cos_compinfo *child_cinfo, 
              struct mem_seg *text_seg, struct mem_seg *data_seg, vaddr_t start_addr)
{
       unsigned long offset, text_offset;
       vaddr_t allocated_data_seg;

       /*
       * Alias the text segment in the new component
       */ 
       for (offset = 0; offset < text_seg->size; offset += PAGE_SIZE) {
              cos_mem_alias_at(child_cinfo, start_addr + offset, 
                                   parent_cinfo, text_seg->addr + offset);
       }
       text_offset = offset;

       /*
       * allocate mem for the data segment and populate it
       */ 
       allocated_data_seg = (vaddr_t) cos_page_bump_allocn(parent_cinfo, data_seg->size);
       assert(allocated_data_seg);
       memcpy((void *)allocated_data_seg, (void *) data_seg->addr, data_seg->size);

       /*
       * Alias the data segment in the new component
       */
       for (offset = 0; offset < data_seg->size; offset += PAGE_SIZE) {
              cos_mem_alias_at(child_cinfo, start_addr + text_offset + offset,
                                   parent_cinfo, allocated_data_seg + offset);
       }

       return allocated_data_seg;
}

static void
copy_caps(struct cos_compinfo *parent_cinfo_l, struct cos_compinfo *fork_cinfo, 
              captblcap_t ckct, pgtblcap_t  ckpt, compcap_t ckcc, 
              sinvcap_t sinv, thdcap_t initthd_cap)
{
       int ret;

       printc("\tCopying pgtbl, captbl, component capabilities\n");
       ret = cos_cap_cpy_at(fork_cinfo, BOOT_CAPTBL_SELF_CT, parent_cinfo_l, ckct);
       assert(ret == 0);

       ret = cos_cap_cpy_at(fork_cinfo, BOOT_CAPTBL_SELF_PT, parent_cinfo_l, ckpt);
       assert(ret == 0);

       ret = cos_cap_cpy_at(fork_cinfo, BOOT_CAPTBL_SELF_COMP, parent_cinfo_l,
                            ckcc);
       assert(ret == 0);

       ret = cos_cap_cpy_at(fork_cinfo, BOOT_CAPTBL_HYP_SINV_CAP, parent_cinfo_l, sinv);
       assert(ret == 0);

       ret = cos_cap_cpy_at(fork_cinfo, BOOT_CAPTBL_SELF_INITHW_BASE,
                            parent_cinfo_l, BOOT_CAPTBL_SELF_INITHW_BASE);
       assert(ret == 0);

       ret = cos_cap_cpy_at(fork_cinfo, BOOT_CAPTBL_SELF_INITTCAP_BASE,
                            parent_cinfo_l, BOOT_CAPTBL_SELF_INITTCAP_BASE);
       assert(ret == 0);

       ret = cos_cap_cpy_at(fork_cinfo, BOOT_CAPTBL_SELF_INITTHD_BASE,
                            parent_cinfo_l, initthd_cap);
       assert(ret == 0);

}

static void
_fwp_fork_cont(struct cos_compinfo *parent_cinfo, struct click_info *chld_info, 
                     vaddr_t allocated_data_seg, unsigned long comp_info_offset)
{
       struct cos_compinfo *child_cinfo = cos_compinfo_get(&chld_info->def_cinfo);
       struct cos_aep_info *child_aep = cos_sched_aep_get(&chld_info->def_cinfo);
       struct cos_component_information *ci;
       compcap_t ckcc;
       captblcap_t ckct;
       pgtblcap_t  ckpt;
       sinvcap_t sinv;

       chld_info->booter_vaddr = allocated_data_seg;

       ci = (void *) allocated_data_seg + comp_info_offset;
       ci->cos_this_spd_id = next_nfid;

       ckct = child_cinfo->captbl_cap;
       ckpt = child_cinfo->pgtbl_cap;
       ckcc = cos_comp_alloc(parent_cinfo, ckct, ckpt,
                                   (vaddr_t) ci->cos_upcall_entry);
       assert(ckcc);
       child_cinfo->comp_cap = ckcc;

       chld_info->initaep = sl_thd_initaep_alloc(&chld_info->def_cinfo, NULL, 0, 0, 0);
       assert(chld_info->initaep);
       sl_thd_param_set(chld_info->initaep, sched_param_pack(SCHEDP_PRIO, LOWEST_PRIORITY));

       _alloc_tls(parent_cinfo, child_cinfo, child_aep->thd, PAGE_SIZE);

       /* Create sinv capability from Userspace to Booter components */
       sinv = cos_sinv_alloc(parent_cinfo, parent_cinfo->comp_cap, (vaddr_t)nf_entry_rets_inv, (vaddr_t)chld_info);
       assert(sinv > 0);

       copy_caps(parent_cinfo, child_cinfo, ckct, ckpt, ckcc, sinv, sl_thd_thdcap(chld_info->initaep));

       cos_thd_switch(sl_thd_thdcap(chld_info->initaep));
}

/*
 * fork a new click component using the configuration file at *conf_str
 */
static void 
fwp_fork(struct click_info *chld_info, struct mem_seg *text_seg, struct mem_seg *data_seg,
              struct mem_seg *ring_seg, int conf_file_idx, unsigned long cinfo_offset, vaddr_t start_addr)
{
       struct cos_compinfo *parent_cinfo = cos_compinfo_get(cos_defcompinfo_curr_get());
       struct cos_compinfo *child_cinfo = cos_compinfo_get(&chld_info->def_cinfo);
       vaddr_t allocated_data_seg;
      
       assert(text_seg);
       assert(data_seg);
       assert(ring_seg);
 
       _fwp_fork(parent_cinfo, chld_info, text_seg, data_seg, ring_seg, conf_file_idx, start_addr);

void
fwp_chain_activate(struct nf_chain *chain)
{
	struct eos_ring *out1, *in2;
	struct click_info *this_nf, *new_nf;
	struct mca_conn *conn;
	(void)conn;
       allocated_data_seg = _alias_click(parent_cinfo, child_cinfo, text_seg, data_seg, start_addr);

       _fwp_fork_cont(parent_cinfo, chld_info, allocated_data_seg, cinfo_offset);
	list_for_each_nf(this_nf, chain) {
		new_nf = this_nf->next;
		if (!new_nf || !new_nf->nd_ring) continue;
		out1 = get_output_ring((void *)this_nf->shmem_addr);
		in2 = get_input_ring((void *)new_nf->shmem_addr);
		conn = mca_conn_create(out1, in2);
	}
	/* TODO: activate threads */
}

static void *
fwp_get_shmem(int shmem_idx)
{
       char *seg;
       assert(shmem_idx < FWP_MAX_MEMSEGS);
       seg = (char *)shmem_addr + shmem_idx * FWP_MEMSEG_SIZE;
       return (void *)seg;
}

static void
fwp_allocate_shmem(vaddr_t start_addr)
{
       vaddr_t empty_page, next_pgd, map_shmem_at, addr;
       struct cos_compinfo *boot_cinfo = cos_compinfo_get(cos_defcompinfo_curr_get());

       /*FIXME An ugly hack*/
       /*
       * We assume that the binary mapped by the llbooter goes in an PGD(4MB)
       * amd we map the SHMEM after this PGD
       */
       next_pgd = round_up_to_pgd_page(start_addr + 1);
       map_shmem_at = ((vaddr_t)cos_get_heap_ptr() > next_pgd) ? (vaddr_t)cos_get_heap_ptr() : next_pgd;
       empty_page = (vaddr_t)cos_page_bump_alloc(boot_cinfo);
       while (addr < map_shmem_at)
              addr = cos_mem_alias(boot_cinfo, boot_cinfo, empty_page);

       shmem_addr = (vaddr_t)cos_page_bump_allocn(boot_cinfo, FWP_MAX_MEMSEGS * FWP_MEMSEG_SIZE);
       printc("shmem addr %lx\n", round_up_to_pgd_page(start_addr + 1));
}

void
fwp_allocate_shmem_sinv(struct cos_compinfo *src_comp, compcap_t dest_compcap)
{
       struct cos_compinfo *boot_cinfo = cos_compinfo_get(cos_defcompinfo_curr_get());
       sinvcap_t next_call_sinvcap;
       int ret;

       /*allocate the sinv capability for next_call*/
       next_call_sinvcap = cos_sinv_alloc(boot_cinfo, dest_compcap, shmem_inv_addr, 0);
       assert(next_call_sinvcap > 0);
       ret = cos_cap_cpy_at(src_comp, BOOT_CAPTBL_NEXT_SINV_CAP,  boot_cinfo, next_call_sinvcap);
       assert(ret == 0);
/* create a chain with single bridge nf */
static struct nf_chain *
fwp_create_chain_bridge()
{
	/* the first nf_id in our system is 2*/
	int nfid, ncid, shmemid;
	struct click_info *nf1;
	struct nf_chain *ret_chain;

	ncid = next_chain_id++;
	ret_chain = &chains[ncid];
	ret_chain->chain_id = ncid;
	ret_chain->tot_nf = 1;
	ret_chain->active_nf = 0;
	ret_chain->next = NULL;

	nfid = next_nfid++;
	nf1 = &chld_infos[nfid];
	nf1->next = NULL;
	nf1->conf_file_idx = 2;
	nf1->nf_id = nfid;
	nf1->nd_thd = 1;
	nf1->nd_ring = 1;
	nf1->nd_sinv = 0;
	nf1->data_seg = &templates[next_template_id++];
	ret_chain->first_nf = ret_chain->last_nf = nf1;

	return ret_chain;
}

/*
 * create a chain of two NFs linked by MCA
 */
static struct nf_chain *
fwp_create_chain1(void){
	int nfid, ncid, shmemid, temid;
	struct click_info *nf1, *nf2;
	struct nf_chain *ret_chain;

	ncid = next_chain_id++;
	ret_chain = &chains[ncid];
	ret_chain->chain_id = ncid;
	ret_chain->tot_nf = 2;
	ret_chain->active_nf = 0;
	ret_chain->next = NULL;

	nfid = next_nfid++;
	nf1 = &chld_infos[nfid];
	nf1->conf_file_idx = 0;
	nf1->nf_id = nfid;
	nf1->nd_thd = 1;
	nf1->nd_ring = 1;
	nf1->nd_sinv = 0;
	nf1->data_seg = &templates[next_template_id++];

	nfid = next_nfid++;
	nf2 = &chld_infos[nfid];
	nf2->next = NULL;
	nf2->conf_file_idx = 1;
	nf2->nf_id = nfid;
	nf2->nd_thd = 1;
	nf2->nd_ring = 1;
	nf2->nd_sinv = 0;
	nf2->data_seg = &templates[next_template_id++];
	nf1->next = nf2;

	ret_chain->first_nf = nf1;
	ret_chain->last_nf = nf2;

	return ret_chain;
}

/*
 * create a chain of two NFs linked by shmem
 */
static struct nf_chain *
fwp_create_chain2(int conf_file1, int conf_file2)
{
	int nfid, ncid, shmemid;
	struct click_info *nf1, *nf2;
	struct nf_chain *ret_chain;

	ncid = next_chain_id++;
	ret_chain = &chains[ncid];
	ret_chain->chain_id = ncid;
	ret_chain->tot_nf = 2;
	ret_chain->active_nf = 0;
	ret_chain->next = NULL;

	nfid = next_nfid++;
	nf1 = &chld_infos[nfid];
	nf1->conf_file_idx = conf_file1;
	nf1->nf_id = nfid;
	nf1->nd_thd = 1;
	nf1->nd_ring = 1;
	nf1->nd_sinv = 1;
	nf1->data_seg = &templates[next_template_id++];

	nfid = next_nfid++;
	nf2 = &chld_infos[nfid];
	nf2->next = NULL;
	nf2->conf_file_idx = conf_file2;
	nf2->nf_id = nfid;
	nf2->nd_thd = 0;
	nf2->nd_ring = 0;
	nf2->nd_sinv = 0;
	nf2->data_seg = &templates[next_template_id++];
	nf1->next = nf2;

	ret_chain->first_nf = nf1;
	ret_chain->last_nf = nf2;

	return ret_chain;
}

/*
 * actually allocate the chain NFs
 */
static struct nf_chain *
fwp_allocate_chain(struct nf_chain *chain, int is_template, int coreid)
{
	struct click_info *this_nf, *new_nf, **last_nf;
	int nfid, ncid, shmemid, last_shmem_id;
	struct nf_chain *ret_chain;
	struct mem_seg *nf_data_seg;

	if (!is_template) {
		ncid = next_chain_id++;
		ret_chain = &chains[ncid];
		*ret_chain = *chain;
		ret_chain->chain_id = ncid;
		last_nf = &(ret_chain->first_nf);

		list_for_each_nf(this_nf, chain) {
			nfid                  = next_nfid++;
			new_nf                = &chld_infos[nfid];
			*last_nf              = new_nf;
			new_nf->conf_file_idx = -1;
			new_nf->nf_id         = nfid;
			new_nf->nd_thd        = this_nf->nd_thd;
			new_nf->nd_ring       = this_nf->nd_ring;
			new_nf->nd_sinv       = this_nf->nd_sinv;
			new_nf->data_seg      = this_nf->data_seg;
			new_nf->next          = NULL;
			ret_chain->last_nf    = new_nf;
			last_nf               = &(new_nf->next);
		}
		chain = ret_chain;
	}

	list_for_each_nf(this_nf, chain) {
		struct mem_seg mem_seg;

		if (this_nf->nd_ring) shmemid = next_shmem_id++;
		else shmemid = last_shmem_id;
		last_shmem_id = shmemid;
		this_nf->shmem_addr = (vaddr_t)fwp_get_shmem(shmemid);
		mem_seg.addr = this_nf->shmem_addr;
		mem_seg.size = FWP_MEMSEG_SIZE;
		if (is_template) nf_data_seg = d_seg;
		else nf_data_seg = this_nf->data_seg;
		fwp_fork(this_nf, t_seg, nf_data_seg, &mem_seg, this_nf->conf_file_idx, cinfo_offset, s_addr);
	}

	if (!is_template) {
		list_for_each_nf(this_nf, chain) {
			if (this_nf->nd_sinv) {
				fwp_allocate_shmem_sinv(cos_compinfo_get(&(this_nf->def_cinfo)),
							cos_compinfo_get(&(this_nf->next->def_cinfo))->comp_cap);
			}
		}
		fwp_chain_put(chain, FWP_CHAIN_CLEANED, coreid);
	}
	return chain;
}

static void
fwp_init(void)
{
       struct cos_compinfo *boot_cinfo = cos_compinfo_get(cos_defcompinfo_curr_get());

       mca_init(boot_cinfo);
       mca_thd = sl_thd_alloc(mca_run, NULL);
       sl_thd_param_set(mca_thd, sched_param_pack(SCHEDP_PRIO, LOWEST_PRIORITY));
       fwp_allocate_shmem(s_addr);
}

void
fwp_test(struct mem_seg *text_seg, struct mem_seg *data_seg, vaddr_t start_addr,
              unsigned long comp_info_offset, vaddr_t sinv_next_call)
{
       struct nf_chain *chain;

       t_seg = text_seg;
       d_seg = data_seg;
       cinfo_offset = comp_info_offset;
       s_addr = start_addr;
       shmem_inv_addr = sinv_next_call;

       fwp_init();

	/* chain = fwp_create_chain1(); */
	/* chain = fwp_create_chain2(3, 4); */
	chain = fwp_create_chain_bridge();
	fwp_allocate_chain(chain, 1, 0);
       
       cos_thd_switch(sl_thd_thdcap(chld_infos[2].initaep));
       //sl_sched_loop();
}
