#pragma once

#include <common.h>
#include <core/http/http-parser.h>

/**
 * @brief Callback function type for parsing specific HTTP headers
 * @param app - [in] Pointer to the global application state structure
 * @param header - [in] Pointer to the HTTP header to be parsed
 * @param priv_data - [out] Pointer to data to be filled by the callback
 */
typedef bool (*http_header_parse_cb_t)(const app_t *app, const phr_header_t *header, void *priv_data);

/**
 * @brief Structure to represent an HTTP header item with its parsing callback
 */
typedef struct {
    const char *name;          ///< Header name
    http_header_parse_cb_t cb; ///< Callback function to parse the header
    void *priv_data;           ///< Pointer to private data for the callback
} http_header_item_t;

/**
 * @brief HTTP enumeration mapping structure
 */
typedef struct {
    uint32_t *pval;           ///< Pointer to the variable where the parsed enum value will be stored
    const char *const *enums; ///< Comma-separated list of enum string values
    uint32_t enums_count;     ///< Number of enum values
} http_enum_t;

/**
 * @brief Parse HTTP headers using specified header items and their callbacks
 * @param app - [in] Pointer to the global application state structure
 * @param headers - [in] Pointer to the array of HTTP headers
 * @param num_headers - [in] Number of HTTP headers
 * @param items - [in] Pointer to the array of HTTP header items with their callbacks
 * @param num_items - [in] Number of HTTP header items
 * @return True if all specified headers are successfully parsed, false otherwise
 */
bool http_parse_headers(const app_t *app, const phr_header_t *headers, uint32_t num_headers,
                        const http_header_item_t *items, uint32_t num_items);

/**
 * @brief Parse a 32-bit integer from an HTTP header
 * @param app - [in] Pointer to the global application state structure
 * @param header - [in] Pointer to the HTTP header to be parsed
 * @param priv_data - [out] Pointer to a uint32_t variable to store the parsed integer
 * @return True if the header is successfully parsed as a 32-bit integer, false otherwise
 */
bool http_parse_int32(const app_t *app, const phr_header_t *header, void *priv_data);

/**
 * @brief Parse an enumeration value from an HTTP header
 * @param app - [in] Pointer to the global application state structure
 * @param header - [in] Pointer to the HTTP header to be parsed
 * @param priv_data - [out] Pointer to an http_enum_t structure to store the parsed enum value
 * @return True if the header is successfully parsed as an enumeration value, false otherwise
 */
bool http_parse_enum(const app_t *app, const phr_header_t *header, void *priv_data);
