#pragma once

#include <common.h>

/**
 * @brief Memory pool entry for fixed size items
 */
typedef struct mem_entry {
    LIST_ENTRY(mem_entry) next; ///< Next entry in the list
    uint64_t used_mask;         ///< Bitmap of used items
    char data[];                ///< Data area
} mem_entry_t;

/**
 * @brief Memory pool for fixed size items
 */
typedef LIST_HEAD(mem_pool, mem_entry) mem_pool_t;

/**
 * @brief Allocate an item from the memory pool
 * @param list - [in] Memory pool
 * @param item_size - [in] Size of each item
 * @return Pointer to the allocated item, or NULL if allocation failed
 */
void *mem_pool_alloc(mem_pool_t *pool, uint32_t item_size);

/**
 * @brief Free an item back to the memory pool
 * @param list - [in] Memory pool
 * @param item - [in] Pointer to the item to free
 * @param item_size - [in] Size of each item
 */
void mem_pool_free(mem_pool_t *pool, void *item, uint32_t item_size);
