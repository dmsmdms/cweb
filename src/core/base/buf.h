#pragma once

#include <common.h>

#define buf_alloc_or_return(buf, size, ret)                                                                            \
    ({                                                                                                                 \
        uint32_t tot_size = size;                                                                                      \
        void *ptr = buf_alloc(buf, tot_size);                                                                          \
        if(ptr == NULL) {                                                                                              \
            log_error("buf_alloc(%u) failed", tot_size);                                                               \
            return ret;                                                                                                \
        }                                                                                                              \
        ptr;                                                                                                           \
    })

/**
 * @brief Generic buffer structure
 */
typedef struct {
    void *data;  ///< Pointer to the buffer data
    size_t size; ///< Size of the buffer data
} buf_t;

/**
 * @brief Buffer generator structure
 */
typedef struct {
    void *data;      ///< Pointer to the buffer data
    uint32_t size;   ///< Size of the buffer data
    uint32_t offset; ///< Current offset in the buffer
} buf_ext_t;

/**
 * @brief Circular buffer structure
 */
typedef struct {
    char *data;         ///< Pointer to the buffer data
    uint32_t off;       ///< Current offset in the buffer
    uint32_t cnt;       ///< Number of elements in the buffer
    uint32_t size;      ///< Total size of the buffer
    uint32_t item_size; ///< Size of each item in the buffer
} buf_circ_t;

/**
 * @brief Initialize an extended buffer
 * @param buf - [out] Pointer to the buffer
 * @param data - [in] Pointer to the buffer data
 * @param size - [in] Size of the buffer data
 */
void buf_init_ext(buf_ext_t *buf, void *data, uint32_t size);

/**
 * @brief Allocate memory from the extended buffer
 * @param buf - [in] Pointer to the extended buffer
 * @param size - [in] Size of memory to allocate
 * @return Pointer to the allocated memory, or NULL if allocation fails
 */
void *buf_alloc(buf_ext_t *buf, uint32_t size);

/**
 * @brief Allocate zero-initialized memory from the extended buffer
 * @param buf - [in] Pointer to the extended buffer
 * @param size - [in] Size of memory to allocate
 * @return Pointer to the allocated memory, or NULL if allocation fails
 */
void *buf_calloc(buf_ext_t *buf, uint32_t size);

/**
 * @brief Initialize a circular buffer
 * @param buf - [in] Pointer to the circular buffer
 * @param data - [in] Pointer to the buffer data
 * @param size - [in] Size of the buffer data
 * @param item_size - [in] Size of each item in the buffer
 */
void buf_circ_init(buf_circ_t *buf, void *data, uint32_t size, uint32_t item_size);

/**
 * @brief Add an item to the circular buffer
 * @param buf - [in] Pointer to the circular buffer
 * @param item - [in] Pointer to the item to add
 */
void buf_circ_add(buf_circ_t *buf, const void *item);

/**
 * @brief Delete the current item from the circular buffer
 * @param buf - [in] Pointer to the circular buffer
 */
void buf_circ_del_cur(buf_circ_t *buf);

/**
 * @brief Get an item from the circular buffer by index
 * @param buf - [in] Pointer to the circular buffer
 * @param index - [in] Index of the item to get (0 = oldest item)
 * @return Pointer to the item at the specified index
 */
const void *buf_circ_get(const buf_circ_t *buf, uint32_t index);

/**
 * @brief Get a float item from the circular buffer by index
 * @param buf - [in] Pointer to the circular buffer
 * @param index - [in] Index of the item to get (0 = oldest item)
 * @return Float value at the specified index
 */
float buf_circ_get_float(const buf_circ_t *buf, uint32_t index);
