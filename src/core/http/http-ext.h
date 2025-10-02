#pragma once

#include <core/http/http-parser.h>
#include <core/http/http-server.h>
#include <core/json/json-gen.h>

/**
 * @brief HTTP parsing error codes
 */
typedef enum {
    HTTP_PARSE_ERR_OK,        ///< Parsing successful
    HTTP_PARSE_ERR_NOT_FOUND, ///< Header not found
    HTTP_PARSE_ERR_INVALID,   ///< Invalid header value
    HTTP_PARSE_ERR_MAX,
} http_parse_err_t;

/**
 * @brief HTTP generation error codes
 */
typedef enum {
    HTTP_GEN_ERR_OK,   ///< Generation successful
    HTTP_GEN_ERR_JSON, ///< JSON generation failed
    HTTP_GEN_ERR_SEND, ///< Sending response failed
    HTTP_GEN_ERR_MAX,
} http_gen_err_t;

/**
 * @brief Callback function type for parsing specific HTTP headers
 * @param header - [in] Pointer to the HTTP header to be parsed
 * @param priv_data - [out] Pointer to data to be filled by the callback
 * @return HTTP parsing error code
 */
typedef http_parse_err_t (*http_header_parse_cb_t)(const phr_header_t *header, void *priv_data);

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
 * @param headers - [in] Pointer to the array of HTTP headers
 * @param num_headers - [in] Number of HTTP headers
 * @param items - [in] Pointer to the array of HTTP header items with their callbacks
 * @param num_items - [in] Number of HTTP header items
 * @return HTTP_PARSE_ERR_OK if all headers are successfully parsed, appropriate error code otherwise
 */
http_parse_err_t http_parse_headers(const phr_header_t *headers, uint32_t num_headers, const http_header_item_t *items,
                                    uint32_t num_items);

/**
 * @brief Parse a 32-bit integer from an HTTP header
 * @param header - [in] Pointer to the HTTP header to be parsed
 * @param priv_data - [out] Pointer to a uint32_t variable to store the parsed integer
 * @return HTTP_PARSE_ERR_OK if the header is successfully parsed as an integer, appropriate error code otherwise
 */
http_parse_err_t http_parse_int32(const phr_header_t *header, void *priv_data);

/**
 * @brief Parse an enumeration value from an HTTP header
 * @param header - [in] Pointer to the HTTP header to be parsed
 * @param priv_data - [out] Pointer to an http_enum_t structure to store the parsed enum value
 * @return HTTP_PARSE_ERR_OK if the header is successfully parsed as an enum, appropriate error code otherwise
 */
http_parse_err_t http_parse_enum(const phr_header_t *header, void *priv_data);

/**
 * @brief Send a JSON response over an HTTP connection
 * @param conn - [in] Pointer to the HTTP connection
 * @param code - [in] HTTP response code
 * @param content_type - [in] Content type of the response
 * @param connection - [in] Connection type (keep-alive or close)
 * @param items - [in] Pointer to the array of JSON generation items
 * @param num_items - [in] Number of JSON generation items
 * @return HTTP_GEN_ERR_OK if the response is successfully sent, appropriate error code otherwise
 */
http_gen_err_t http_resp_json(shttp_conn_t *conn, shttp_resp_code_t code, shttp_content_type_t content_type,
                              shttp_connection_t connection, const json_gen_item_t *items, uint32_t num_items);
