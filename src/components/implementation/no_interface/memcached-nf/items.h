/*@null@*/
void item_init();
item *do_item_alloc(char *key, const size_t nkey, const int flags, const rel_time_t exptime, const int nbytes, const uint32_t cur_hv);
void item_free(item *it);

int  do_item_link(item *it, const uint32_t hv);     /** may fail if transgresses limits */
void do_item_unlink(item *it, const uint32_t hv);
void do_item_remove(item *it);
void do_item_remove_free(item *it);
void do_item_update(item *it);   /** update LRU time to current and reposition */
int  do_item_replace(item *it, item *new_it, const uint32_t hv);

item *do_item_get(const char *key, const size_t nkey, const uint32_t hv);
item *do_item_touch(const char *key, const size_t nkey, uint32_t exptime, const uint32_t hv);
