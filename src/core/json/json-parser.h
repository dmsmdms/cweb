#pragma once

#include <common.h>
#include <core/json/jsmn.h>

/**
 * @brief Callback function type for JSON parsing
 * @param app - [in] Pointer to the application structure
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Private data passed to the callback
 * @return True on success, false on failure
 */
typedef bool (*json_parse_cb_t)(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data);

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
 * @param app - [in] Pointer to the application structure
 * @param json - [in] JSON string
 * @param json_size - [in] Size of JSON string
 * @param toks - [out] Array of tokens
 * @param num_tok - [in] Number of tokens in the array
 * @return True on success, false on failure
 */
bool json_parse_wrap(app_t *app, const char *json, uint32_t json_size, const json_item_t *items, uint32_t num_items);

/**
 * @brief Parse JSON object
 * @param app - [in] Pointer to the application structure
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param items - [in] Array of JSON items to parse
 * @param num_items - [in] Number of items in the array
 * @return True on success, false on failure
 */
bool json_parse_obj(app_t *app, const jsmntok_t *cur, const char *json, const json_item_t *items, uint32_t num_items);

/**
 * @brief Parse array of JSON objects
 * @param app - [in] Pointer to the application structure
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param cb - [in] Callback function to parse each item in the array
 * @param priv_data - [in] Private data passed to the callback
 * @return True on success, false on failure
 */
bool json_parse_arr(app_t *app, const jsmntok_t *cur, const char *json, json_parse_cb_t cb, void *priv_data);

/**
 * @brief Parse 32-bit integer from JSON token
 * @param app - [in] Pointer to the application structure
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Pointer to integer where the result will be stored
 * @return True on success, false on failure
 */
bool json_parse_int32(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data);

/**
 * @brief Parse 64-bit integer from JSON token
 * @param app - [in] Pointer to the application structure
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Pointer to int64_t where the result will be stored
 * @return True on success, false on failure
 */
bool json_parse_int64(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data);

/**
 * @brief Parse boolean from JSON token
 * @param app - [in] Pointer to the application structure
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Pointer to boolean where the result will be stored
 * @return True on success, false on failure
 */
bool json_parse_bool(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data);

/**
 * @brief Parse string from JSON token
 * @param app - [in] Pointer to the application structure
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Pointer to char array where the result will be stored
 * @return True on success, false on failure
 */
bool json_parse_str(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data);

/**
 * @brief Parse string from JSON token and allocate memory for it
 * @param app - [in] Pointer to the application structure
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Pointer to char pointer where the allocated string will be stored
 * @return True on success, false on failure
 */
bool json_parse_pstr(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data);

/**
 * @brief Parse enumeration from JSON token
 * @param app - [in] Pointer to the application structure
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Pointer to structure containing enum mapping and output pointer
 * @return True on success, false on failure
 */
bool json_parse_enum(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data);
