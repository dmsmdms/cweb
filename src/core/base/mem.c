#include <global.h>

#ifdef CONFIG_HTML_PARSER
    #define GLOBAL_BUF_SIZE (128 * 1024 * 1024)
#else
    #define GLOBAL_BUF_SIZE (4 * 1024 * 1024)
#endif
#define MEM_POOL_BLOCK_ITEMS 64

bool mem_init(app_t *app)
{
    mem_t *mem = &app->mem;
    mem->data = malloc(GLOBAL_BUF_SIZE);
    if(mem->data == NULL) {
        log_error("buffer alloc failed, size=%u", GLOBAL_BUF_SIZE);
        return false;
    }
    mem->offset = 0;
    return true;
}

void mem_free(app_t *app)
{
    mem_t *mem = &app->mem;
    if(mem->offset) {
        log_warn("memory leak detected size=%u", mem->offset);
    }
    free(mem->data);
    mem->data = NULL;
}

void *mem_alloc(app_t *app, const char *name, uint32_t size)
{
    mem_t *mem = &app->mem;
    size = ROUND_UP(size, sizeof(void *));
    if((mem->offset + size) > GLOBAL_BUF_SIZE) {
        log_error("buffer overflow, name=%s offset=%u size=%u", name, mem->offset, size);
        return NULL;
    }
    void *ptr = mem->data + mem->offset;
    mem->offset += size;
    return ptr;
}

void *mem_calloc(app_t *app, const char *name, uint32_t size)
{
    void *ptr = mem_alloc(app, name, size);
    if(ptr) {
        bzero(ptr, size);
    }
    return ptr;
}

uint32_t mem_get_offset(const app_t *app)
{
    return app->mem.offset;
}

void mem_put_offset(app_t *app, uint32_t offset)
{
    app->mem.offset = offset;
}

void mem_pool_init(app_t *app, mem_pool_t *pool, const char *name, uint32_t item_size)
{
    pool->app = app;
    pool->name = name;
    pool->item_size = item_size;
}

void *mem_pool_alloc(mem_pool_t *pool)
{
    const app_t *app = pool->app;
    mem_pool_block_t *block;
    SLIST_FOREACH(block, &pool->blocks, entry)
    {
        uint32_t free_idx = ffsll(~block->used_mask);
        if(free_idx != 0) {
            free_idx--;
            BIT_SET(block->used_mask, free_idx);
            return block->data + (free_idx * pool->item_size);
        }
    }
    block = malloc(sizeof(mem_pool_block_t) + (pool->item_size * MEM_POOL_BLOCK_ITEMS));
    if(block == NULL) {
        log_error("alloc failed, name=%s", pool->name);
        return NULL;
    }
    log_debug("alloc block, name=%s", pool->name);
    SLIST_INSERT_HEAD(&pool->blocks, block, entry);
    block->used_mask = 1;
    return block->data;
}

void *mem_pool_calloc(mem_pool_t *pool)
{
    void *ptr = mem_pool_alloc(pool);
    if(ptr) {
        bzero(ptr, pool->item_size);
    }
    return ptr;
}

void mem_pool_free(mem_pool_t *pool, const void *ptr)
{
    if(ptr == NULL) {
        return;
    }
    const app_t *app = pool->app;
    const char *cptr = ptr;
    mem_pool_block_t *block;
    SLIST_FOREACH(block, &pool->blocks, entry)
    {
        const char *end = block->data + (pool->item_size * MEM_POOL_BLOCK_ITEMS);
        if(cptr >= block->data && cptr < end) {
            uint32_t idx = (cptr - block->data) / pool->item_size;
            if(!BIT_CHECK(block->used_mask, idx)) {
                log_warn("double free detected, name=%s", pool->name);
            } else {
                BIT_CLEAR(block->used_mask, idx);
            }
            return;
        }
    }
    log_warn("pointer not in pool, name=%s", pool->name);
}

void mem_pool_destroy(mem_pool_t *pool)
{
    const app_t *app = pool->app;
    mem_pool_block_t *block = pool->blocks.slh_first;
    while(block) {
        mem_pool_block_t *next = block->entry.sle_next;
        if(block->used_mask) {
            log_warn("memory leak detected, name=%s", pool->name);
        }
        free(block);
        block = next;
    }
    SLIST_INIT(&pool->blocks);
}
