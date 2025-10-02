#pragma once

#include <core/base/str.h>
#include <core/json/jsmn.h>

/**
 * @brief JSON generation error codes
 */
typedef enum {
    JSON_GEN_ERR_OK,       ///< No error
    JSON_GEN_ERR_OVERFLOW, ///< Buffer overflow
    JSON_GEN_ERR_MAX,
} json_gen_err_t;

/**
 * @brief JSON generation callback type
 * @param out - [out] Pointer to output string
 * @param priv_data - [in] Pointer to private data
 * @return JSON_GEN_ERR_OK on success, error code otherwise
 */
typedef json_gen_err_t (*json_gen_cb_t)(str_buf_t *out, const void *priv_data);

/**
 * @brief JSON generation item structure
 */
typedef struct {
    const char *name;      ///< Name of the JSON item
    json_gen_cb_t cb;      ///< JSON generation callback
    const void *priv_data; ///< Pointer to private data
} json_gen_item_t;

/**
 * @brief JSON array generation structure
 */
typedef struct {
    json_gen_cb_t cb;   ///< JSON generation callback for array items
    const void *arr;    ///< Pointer to array of items
    uint32_t count;     ///< Number of items in the array
    uint32_t item_size; ///< Size of each item in the array
} json_gen_arr_t;

/**
 * @brief JSON string array structure
 */
typedef struct {
    const char *const *arr; ///< Pointer to array of strings
    uint32_t size;          ///< Amount of strings in the array
} json_str_arr_t;

/**
 * @brief Generate JSON object
 * @param out - [out] Pointer to output string
 * @param items - [in] Pointer to array of JSON items
 * @param num_items - [in] Number of JSON items
 * @return JSON_GEN_ERR_OK on success, error code otherwise
 */
json_gen_err_t json_gen_obj(str_buf_t *out, const json_gen_item_t *items, uint32_t num_items);

/**
 * @brief Generate JSON array
 * @param out - [out] Pointer to output string
 * @param info - [in] Pointer to JSON array generation structure
 * @return JSON_GEN_ERR_OK on success, error code otherwise
 */
json_gen_err_t json_gen_arr(str_buf_t *out, const json_gen_arr_t *info);

/**
 * @brief Generate JSON float
 * @param out - [out] Pointer to output string
 * @param priv_data - [in] Pointer to private data (float pointer)
 * @return JSON_GEN_ERR_OK on success, error code otherwise
 */
json_gen_err_t json_gen_float(str_buf_t *out, const void *priv_data);

/**
 * @brief Generate JSON uint64
 * @param out - [out] Pointer to output string
 * @param priv_data - [in] Pointer to private data (uint64_t pointer)
 * @return JSON_GEN_ERR_OK on success, error code otherwise
 */
json_gen_err_t json_gen_uint64(str_buf_t *out, const void *priv_data);

/**
 * @brief Generate JSON uint8
 * @param out - [out] Pointer to output string
 * @param priv_data - [in] Pointer to private data (uint8_t pointer)
 * @return JSON_GEN_ERR_OK on success, error code otherwise
 */
json_gen_err_t json_gen_uint8(str_buf_t *out, const void *priv_data);

/**
 * @brief Generate JSON string
 * @param out - [out] Pointer to output string
 * @param priv_data - [in] Pointer to private data (string to generate)
 * @return JSON_GEN_ERR_OK on success, error code otherwise
 */
json_gen_err_t json_gen_str(str_buf_t *out, const void *priv_data);
