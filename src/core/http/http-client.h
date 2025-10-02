#pragma once

#include <core/base/str.h>
#include <core/base/buf.h>

/**
 * @brief Enumeration for HTTP client error codes
 */
typedef enum {
    CHTTP_ERR_OK,        ///< No error
    CHTTP_ERR_RESP,      ///< HTTP response code indicates failure
    CHTTP_ERR_CURL,      ///< CURL operation failed
    CHTTP_ERR_MEM_ALLOC, ///< Memory allocation failed
    CHTTP_ERR_MAX,
} chttp_err_t;

/**
 * @brief Enumeration for HTTP mime types for POST request
 */
typedef enum {
    CHTTP_MIME_TYPE_DATA, ///< String data
    CHTTP_MIME_TYPE_FILE, ///< File data
    CHTTP_MIME_TYPE_MAX,
} chttp_mime_type_t;

/**
 * @brief Structure to represent an HTTP mime part
 */
typedef struct {
    chttp_mime_type_t type; ///< Type of the mime part (data or file)
    const char *name;       ///< Name of the mime part
    const char *data;       ///< String data or file path
} chttp_mime_t;

/**
 * @brief Structure to represent an HTTP client response
 */
typedef struct {
    chttp_err_t err; ///< Error code of the HTTP response
    const char *url; ///< URL of the HTTP request
    str_t body;      ///< Body of the HTTP response
    void *user_data; ///< User private data
} chttp_resp_t;

/**
 * @brief Callback function type for handling HTTP client responses
 * @param resp - [in] Pointer to the HTTP client response structure
 */
typedef void (*chttp_resp_cb_t)(const chttp_resp_t *resp);

/**
 * @brief Initialize the HTTP multi-connection client
 * @return CHTTP_ERR_OK on success, error code otherwise
 */
chttp_err_t chttp_init(void);

/**
 * @brief Destroy the HTTP multi-connection client and free resources
 */
void chttp_destroy(void);

/**
 * @brief Perform an asynchronous HTTP GET request
 * @param url - [in] URL to send the GET request to
 * @param cb - [in] Callback function to handle the HTTP response
 * @param user_data - [in] User private  data to be passed to the callback function
 * @return CHTTP_ERR_OK on success, error code otherwise
 */
chttp_err_t chttp_get(const char *url, chttp_resp_cb_t cb, const buf_t *user_data);

/**
 * @brief Perform an asynchronous HTTP POST request
 * @param url - [in] URL to send the POST request to
 * @param mimes - [in] Array of mime parts to include in the POST request
 * @param mimes_size - [in] Number of mime parts in the array
 * @param cb - [in] Callback function to handle the HTTP response
 * @param user_data - [in] User private  data to be passed to the callback function
 * @return CHTTP_ERR_OK on success, error code otherwise
 */
chttp_err_t chttp_post(const char *url, const chttp_mime_t *mimes, uint32_t mimes_count, chttp_resp_cb_t cb,
                       const buf_t *user_data);
