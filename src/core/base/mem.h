#pragma once

#include <common.h>

/**
 * @brief Block of momory pool
 */
typedef struct mem_pool_block {
    SLIST_ENTRY(mem_pool_block) entry; ///< Singly linked list entry
    uint64_t used_mask;                ///< Bitmask to track used items in block
    char data[];                       ///< Flexible array member to hold pool items
} mem_pool_block_t;

/**
 * @brief List of memory pool blocks
 */
typedef SLIST_HEAD(mem_pool_list, mem_pool_block) mem_pool_list_t;

/**
 * @brief Memory pool instance
 */
typedef struct {
    mem_pool_list_t blocks; ///< List of memory pool blocks
    const app_t *app;       ///< Pointer to the application instance
    const char *name;       ///< Name of memory pool
    uint32_t item_size;     ///< Size of one item in block
} mem_pool_t;

/**
 * @brief Structure to manage a temporary memory buffer
 */
typedef struct {
    char *data;         //!< Temporary buffer
    uint32_t offset;    //!< Current offset in the buffer
    uint32_t get_count; //!< Number of get operations
} mem_t;

/**
 * @brief Initialize the memory buffer
 * @param app - [in] Pointer to the application instance
 * @return true on success, false on failure
 */
bool mem_init(app_t *app);

/**
 * @brief Free the memory buffer
 * @param app - [in] Pointer to the application instance
 */
void mem_free(app_t *app);

/**
 * @brief Allocate a block of memory from the buffer
 * @param app - [in] Pointer to the application instance
 * @param name - [in] Name of the memory allocation
 * @param size - [in] Size of memory to allocate
 * @return Pointer to the allocated memory, or NULL on failure
 */
void *mem_alloc(app_t *app, const char *name, uint32_t size);

/**
 * @brief Allocate a block of memory from the buffer and fill with zero
 * @param app - [in] Pointer to the application instance
 * @param name - [in] Name of the memory allocation
 * @param size - [in] Size of memory to allocate
 * @return Pointer to the allocated memory, or NULL on failure
 */
void *mem_calloc(app_t *app, const char *name, uint32_t size);

/**
 * @brief Get the current offset in the memory buffer
 * @param app - [in] Pointer to the application instance
 * @return Current offset in the buffer
 */
uint32_t mem_get_offset(app_t *app);

/**
 * @brief Set the current offset in the memory buffer
 * @param app - [in] Pointer to the application instance
 * @param offset - [in] New offset value
 */
void mem_put_offset(app_t *app, uint32_t offset);

/**
 * @brief Set initial values on memory pool
 * @param app - [in] Pointer to the application instance
 * @param pool - [out] Instance of memory pool
 * @param name - [in] Name of memory pool
 * @param item_size - [in] Memory pool item size
 */
void mem_pool_init(app_t *app, mem_pool_t *pool, const char *name, uint32_t item_size);

/**
 * @brief Allocate memory from memory pool
 * @param pool - [in] Instance of memory pool
 * @param size - [in] Size of memory to allocate
 * @return Pointer to the allocated memory, or NULL on failure
 */
void *mem_pool_alloc(mem_pool_t *pool);

/**
 * @brief Allocate memory from memory pool and fill with zero
 * @param pool - [in] Instance of memory pool
 * @param size - [in] Size of memory to allocate
 * @return Pointer to the allocated memory, or NULL on failure
 */
void *mem_pool_calloc(mem_pool_t *pool);

/**
 * @brief Free memory allocated from memory pool
 * @param pool - [in] Instance of memory pool
 * @param ptr - [in] Pointer to the allocated memory
 */
void mem_pool_free(mem_pool_t *pool, const void *ptr);

/**
 * @brief Release memory allocated from this pool
 * @param pool - [in] Instance of memory pool
 */
void mem_pool_destroy(mem_pool_t *pool);
