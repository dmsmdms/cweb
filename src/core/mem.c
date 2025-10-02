#include <core/mem.h>

#define ENTRY_ITEMS_COUNT (8 * sizeof((mem_entry_t){}.used_mask))

void *mem_pool_alloc(mem_pool_t *pool, uint32_t item_size)
{
    mem_entry_t *entry = NULL;
    LIST_FOREACH(entry, pool, next)
    {
        uint32_t idx = __builtin_ffs(~entry->used_mask);
        if(idx > 0) {
            idx--;
            BIT_SET(entry->used_mask, idx);
            return entry->data + (item_size * idx);
        }
    }
    entry = malloc(sizeof(mem_entry_t) + (item_size * ENTRY_ITEMS_COUNT));
    if(entry == NULL) {
        return NULL;
    }
    LIST_INSERT_HEAD(pool, entry, next);
    entry->used_mask = 1;
    return entry->data;
}

void mem_pool_free(mem_pool_t *pool, void *item, uint32_t item_size)
{
    mem_entry_t *entry = NULL;
    LIST_FOREACH(entry, pool, next)
    {
        void *start = entry->data;
        void *end = entry->data + (item_size * ENTRY_ITEMS_COUNT);
        if(item >= start && item < end) {
            uint32_t idx = ((char *)item - (char *)start) / item_size;
            BIT_CLEAR(entry->used_mask, idx);
            return;
        }
    }
}
