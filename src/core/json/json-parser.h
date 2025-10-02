#pragma once

#include <core/base/str.h>
#include <core/json/jsmn.h>

/**
 * @brief JSON parsing error codes
 */
typedef enum {
    JSON_PARSE_ERR_OK,      ///< No error
    JSON_PARSE_ERR_DECODE,  ///< Error during decoding
    JSON_PARSE_ERR_INVALID, ///< Invalid JSON
    JSON_PARSE_ERR_NO_KEY,  ///< Key not found
    JSON_PARSE_ERR_CONVERT, ///< Conversion error
    JSON_PARSE_ERR_NO_MEM,  ///< Not enough memory
    JSON_PARSE_MAX,
} json_parse_err_t;

/**
 * @brief Callback function type for JSON parsing
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Private data passed to the callback
 * @return JSON_PARSE_ERR_OK on success, error code on failure
 */
typedef json_parse_err_t (*json_parse_cb_t)(const jsmntok_t *cur, const char *json, void *priv_data);

/**
 * @brief JSON item structure
 */
typedef struct {
    const char *name;   ///< Name of the JSON item
    json_parse_cb_t cb; ///< Callback function to decode the item
    void *priv_data;    ///< Private data for the callback
} json_item_t;

/**
 * @brief JSON enumeration mapping structure
 */
typedef struct {
    uint32_t *pval;           ///< Pointer to the variable where the parsed enum value will be stored
    const char *const *enums; ///< Comma-separated list of enum string values
    uint32_t enums_count;     ///< Number of enum values
} json_enum_t;

/**
 * @brief Wrapper for jsmn_parse with default parameters
 * @param json - [in] JSON string
 * @param json_size - [in] Size of JSON string
 * @param toks - [out] Array of tokens
 * @param num_tok - [in] Number of tokens in the array
 * @return JSON_PARSE_ERR_OK on success, error code on failure
 */
json_parse_err_t json_parse(const char *json, uint32_t json_size, const json_item_t *items, uint32_t num_items);

/**
 * @brief Parse JSON object
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param items - [in] Array of JSON items to parse
 * @param num_items - [in] Number of items in the array
 * @return JSON_PARSE_ERR_OK on success, error code on failure
 */
json_parse_err_t json_parse_obj(const jsmntok_t *cur, const char *json, const json_item_t *items, uint32_t num_items);

/**
 * @brief Parse array of JSON objects
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param cb - [in] Callback function to parse each item in the array
 * @param priv_data - [in] Private data passed to the callback
 * @return JSON_PARSE_ERR_OK on success, error code on failure
 */
json_parse_err_t json_parse_arr(const jsmntok_t *cur, const char *json, json_parse_cb_t cb, void *priv_data);

/**
 * @brief Parse float from JSON token
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Pointer to float where the result will be stored
 * @return JSON_PARSE_ERR_OK on success, error code on failure
 */
json_parse_err_t json_parse_float(const jsmntok_t *cur, const char *json, void *priv_data);

/**
 * @brief Parse 32-bit integer from JSON token
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Pointer to integer where the result will be stored
 * @return JSON_PARSE_ERR_OK on success, error code on failure
 */
json_parse_err_t json_parse_int32(const jsmntok_t *cur, const char *json, void *priv_data);

/**
 * @brief Parse 64-bit integer from JSON token
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Pointer to int64_t where the result will be stored
 * @return JSON_PARSE_ERR_OK on success, error code on failure
 */
json_parse_err_t json_parse_int64(const jsmntok_t *cur, const char *json, void *priv_data);

/**
 * @brief Parse boolean from JSON token
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Pointer to boolean where the result will be stored
 * @return JSON_PARSE_ERR_OK on success, error code on failure
 */
json_parse_err_t json_parse_bool(const jsmntok_t *cur, const char *json, void *priv_data);

/**
 * @brief Parse string from JSON token and allocate memory for it
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Pointer to char pointer where the allocated string will be stored
 * @return JSON_PARSE_ERR_OK on success, error code on failure
 */
json_parse_err_t json_parse_pstr(const jsmntok_t *cur, const char *json, void *priv_data);

/**
 * @brief Parse enumeration from JSON token
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Pointer to structure containing enum mapping and output pointer
 * @return JSON_PARSE_ERR_OK on success, error code on failure
 */
json_parse_err_t json_parse_enum(const jsmntok_t *cur, const char *json, void *priv_data);

/**
 * @brief Parse JSON string and duplicate it if it inside the buffer
 * @param pdata - [out] Pointer to store duplicated strings
 * @param buf - [in] Buffer to duplicate strings into
 * @param items - [in] Array of JSON items to parse
 * @param num_items - [in] Number of items in the array
 * @return JSON_PARSE_ERR_OK on success, error code on failure
 */
json_parse_err_t json_parse_pstr_dup(char **pdata, const str_t *buf, const json_item_t *items, uint32_t num_items);
