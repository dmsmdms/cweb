#pragma once

#include <common.h>

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
 * @brief Allocate memory from the extended buffer
 * @param buf - [in] Pointer to the extended buffer
 * @param size - [in] Size of memory to allocate
 * @return Pointer to the allocated memory, or NULL if allocation fails
 */
void *buf_alloc(buf_ext_t *buf, size_t size);

/**
 * @brief Allocate zero-initialized memory from the extended buffer
 * @param buf - [in] Pointer to the extended buffer
 * @param size - [in] Size of memory to allocate
 * @return Pointer to the allocated memory, or NULL if allocation fails
 */
void *buf_calloc(buf_ext_t *buf, size_t size);
