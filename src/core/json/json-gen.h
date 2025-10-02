#pragma once

#include <core/base/str.h>
#include <core/json/jsmn.h>

/**
 * @brief JSON generation callback type
 * @param app - [in] Pointer to application structure
 * @param out - [out] Pointer to output string
 * @param priv_data - [in] Pointer to private data
 */
typedef bool (*json_gen_cb_t)(app_t *app, str_t *out, const void *priv_data);

/**
 * @brief JSON generation item structure
 */
typedef struct {
    const char *name;      ///< Name of the JSON item
    json_gen_cb_t cb;      ///< JSON generation callback
    const void *priv_data; ///< Pointer to private data
} json_gen_item_t;

/**
 * @brief JSON string array structure
 */
typedef struct {
    const char *const *arr; ///< Pointer to array of strings
    uint32_t size;          ///< Amount of strings in the array
} json_str_arr_t;

/**
 * @brief Generate JSON from items
 * @param app - [in] Pointer to application structure
 * @param out - [out] Pointer to output string
 * @param items - [in] Pointer to array of JSON items
 * @param num_items - [in] Number of JSON items
 * @return Pointer to end of generated JSON string in buffer
 */
bool json_gen_wrap(app_t *app, str_t *out, const json_gen_item_t *items, uint32_t num_items);

/**
 * @brief Generate JSON object
 * @param app - [in] Pointer to application structure
 * @param out - [out] Pointer to output string
 * @param items - [in] Pointer to array of JSON items
 * @param num_items - [in] Number of JSON items
 * @return True on success, false on failure
 */
bool json_gen_obj(app_t *app, str_t *out, const json_gen_item_t *items, uint32_t num_items);

/**
 * @brief Generate JSON float
 * @param app - [in] Pointer to application structure
 * @param out - [out] Pointer to output string
 * @param priv_data - [in] Pointer to private data (float pointer)
 * @return True on success, false on failure
 */
bool json_gen_float(app_t *app, str_t *out, const void *priv_data);

/**
 * @brief Generate JSON uint64
 * @param app - [in] Pointer to application structure
 * @param out - [out] Pointer to output string
 * @param priv_data - [in] Pointer to private data (uint64_t pointer)
 * @return True on success, false on failure
 */
bool json_gen_uint64(app_t *app, str_t *out, const void *priv_data);

/**
 * @brief Generate JSON uint8
 * @param app - [in] Pointer to application structure
 * @param out - [out] Pointer to output string
 * @param priv_data - [in] Pointer to private data (uint8_t pointer)
 * @return True on success, false on failure
 */
bool json_gen_uint8(app_t *app, str_t *out, const void *priv_data);

/**
 * @brief Generate JSON string
 * @param app - [in] Pointer to application structure
 * @param out - [out] Pointer to output string
 * @param priv_data - [in] Pointer to private data (string to generate)
 * @return True on success, false on failure
 */
bool json_gen_str(app_t *app, str_t *out, const void *priv_data);

/**
 * @brief Generate JSON string array
 * @param app - [in] Pointer to application structure
 * @param out - [out] Pointer to output string
 * @param priv_data - [in] Pointer to private data (json_str_arr_t)
 * @return True on success, false on failure
 */
bool json_gen_str_arr(app_t *app, str_t *out, const void *priv_data);
