/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <nf_hypercall.h>
#include "memcached.h"

#define MC_ITEM_ALLOC_SZ 1024
#define MC_ITEM_MAX_NUM 100
static char item_mem[MC_ITEM_MAX_NUM+1][MC_ITEM_ALLOC_SZ];
static void *item_free_list;
static int item_free_idx;

void
item_init()
{
    item_free_idx = 0;
    memset(item_mem, 0, MC_ITEM_ALLOC_SZ);
    item_free_list = NULL;
}

static inline void *
__item_alloc(int sz)
{
    void *ret;

    assert(sz <= MC_ITEM_ALLOC_SZ);
    if (!item_free_list) {
        assert(item_free_idx <= MC_ITEM_MAX_NUM);
        return (void *)item_mem[item_free_idx++];
    } else {
        assert(item_free_list);
        ret = item_free_list;
        item_free_list = NULL;
        return ret;
    }
}

static inline void
__item_free(void *obj)
{
    assert(!item_free_list);
    item_free_list = obj;
}

/**
 * Generates the variable-sized part of the header for an object.
 *
 * key     - The key
 * nkey    - The length of the key
 * flags   - key flags
 * nbytes  - Number of bytes to hold value and addition CRLF terminator
 * suffix  - Buffer for the "VALUE" line suffix (flags, size).
 * nsuffix - The length of the suffix is stored here.
 *
 * Returns the total size of the header.
 */
static size_t
item_make_header(const uint8_t nkey, const int flags, const int nbytes, char *suffix, uint8_t *nsuffix)
{
    /* suffix is defined at 40 chars elsewhere.. */
    *nsuffix = (uint8_t) snprintf(suffix, 40, " %d %d\r\n", flags, nbytes - 2);
    return sizeof(item) + nkey + *nsuffix + nbytes;
}

/*@null@*/
item *
do_item_alloc(char *key, const size_t nkey, const int flags,
                    const rel_time_t exptime, const int nbytes,
                    const uint32_t cur_hv)
{
    uint8_t nsuffix;
    item *it = NULL;
    char suffix[40];
    size_t ntotal = item_make_header(nkey + 1, flags, nbytes, suffix, &nsuffix);

    it = (item *)__item_alloc(ntotal);
    if (it == NULL) {
        printc(">>>>>>>>>>>>>>>>>>> WARNING: NO memory allocated after eviction!\n");
        return NULL;
    }
    assert(it->slabs_clsid == 0);

    it->next        = it->prev = it->h_next = 0;
    it->slabs_clsid = 0;
    it->it_flags    = 0;
    it->nkey        = nkey;
    it->nbytes      = nbytes;
    it->exptime     = 0; //exptime; /* disable expiration. */
    it->nsuffix     = nsuffix;
    memcpy(ITEM_key(it), key, nkey);
    memcpy(ITEM_suffix(it), suffix, (size_t)nsuffix);

    return it;
}

int
do_item_link(item *it, const uint32_t hv)
{
    assert((it->it_flags & (ITEM_LINKED|ITEM_SLABBED)) == 0);
    it->it_flags |= ITEM_LINKED;
    assoc_insert(it, hv);

    return 1;
}

void
item_free(item *it)
{
    __item_free((void *)it);
    return 0;
}

/* this ignores the refcnt. */
void
do_item_remove_free(item *it)
{
    assert((it->it_flags & ITEM_SLABBED) == 0);

    item_free(it);
}

/** wrapper around assoc_find which does the lazy expiration logic */
item *
do_item_get(const char *key, const size_t nkey, const uint32_t hv)
{
    item *it = assoc_find(key, nkey, hv);

    return it;
}

int
do_item_replace(item *old, item *new, const uint32_t hv)
{
    assert(old->it_flags & ITEM_LINKED);    
    assert((new->it_flags & (ITEM_LINKED|ITEM_SLABBED)) == 0);
    new->it_flags |= ITEM_LINKED;
    assoc_replace(old, new, hv);
    old->it_flags &= ~ITEM_LINKED;
    do_item_remove_free(old);

    return 1;
}

