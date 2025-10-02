#pragma once

#include <core/base/mem.h>
#include <curl/curl.h>

#define HTTP_USER_DATA_SIZE 20

/**
 * @brief Enumeration for HTTP mime types for POST request
 */
typedef enum {
    HTTP_MIME_TYPE_DATA, ///< String data
    HTTP_MIME_TYPE_FILE, ///< File data
    HTTP_MIME_TYPE_MAX,
} http_mime_type_t;

/**
 * @brief Structure to represent an HTTP mime part
 */
typedef struct {
    http_mime_type_t type; ///< Type of the mime part (data or file)
    const char *name;      ///< Name of the mime part
    const char *data;      ///< String data or file path
} http_mime_t;

/**
 * @brief Structure to represent an HTTP response
 */
typedef struct {
    app_t *app;       ///< Global application state structure
    const char *url;  ///< URL of the HTTP request
    const char *data; ///< Response data
    uint32_t size;    ///< Size of the response data
    void *priv_data;  ///< Private data associated with the request
} http_resp_t;

/**
 * @brief Callback function type for handling HTTP responses
 * @param resp - [in] Pointer to the HTTP response structure
 */
typedef void (*http_resp_cb_t)(const http_resp_t *resp);

/**
 * @brief Linked list to manage multiple HTTP requests
 */
typedef LIST_HEAD(http_req_list, http_req) http_req_list_t;

/**
 * @brief Instance of the HTTP multi-connection client
 */
typedef struct {
    http_req_list_t req_list; ///< List of active HTTP requests
    ev_timer timer;           ///< Timer for managing connection timeouts
    CURLM *multi;             ///< CURL multi-handle for managing multiple connections
    mem_pool_t req_pool;      ///< Memory pool for HTTP requests
    mem_pool_t data_pool;     ///< Memory pool for HTTP responces
    mem_pool_t sock_pool;     ///< Memory pool for socket event listeners
} http_client_t;

/**
 * @brief Structure to represent an HTTP request
 */
typedef struct http_req {
    LIST_ENTRY(http_req) entry;          ///< Linked list entry for managing multiple requests
    app_t *app;                          ///< Pointer to the application instance
    CURL *curl;                          ///< CURL easy handle for the request
    curl_mime *mime;                     ///< CURL mime handle for POST requests
    http_resp_cb_t resp_cb;              ///< Callback function to handle the response
    void *priv_data;                     ///< Private data associated with the request
    char *data;                          ///< Buffer to store the response data
    char user_data[HTTP_USER_DATA_SIZE]; ///< Aligned by 8 bytes user data
    uint32_t offset;                     ///< Current offset in the response data buffer
} http_req_t;

/**
 * @brief Initialize the HTTP multi-connection client
 * @param app - [in] Pointer to the application instance
 * @return true on success, false otherwise
 */
bool http_client_init(app_t *app);

/**
 * @brief Destroy the HTTP multi-connection client and free resources
 * @param app - [in] Pointer to the application instance
 */
void http_client_destroy(app_t *app);

/**
 * @brief Perform an asynchronous HTTP GET request
 * @param app - [in] Pointer to the application instance
 * @param url - [in] URL to send the GET request to
 * @param cb - [in] Callback function to handle the HTTP response
 * @param priv_data - [in] Private data to be passed to the callback function
 * @param data_size - [in] Size of the private data to allocate memory for it
 * @return true on success, false otherwise
 */
bool http_get(app_t *app, const char *url, http_resp_cb_t cb, void *priv_data, uint32_t data_size);

/**
 * @brief Perform an asynchronous HTTP POST request
 * @param app - [in] Pointer to the application instance
 * @param url - [in] URL to send the POST request to
 * @param mimes - [in] Array of mime parts to include in the POST request
 * @param mimes_size - [in] Number of mime parts in the array
 * @param cb - [in] Callback function to handle the HTTP response
 * @param priv_data - [in] Private data to be passed to the callback function
 * @param data_size - [in] Size of the private data to allocate memory for it
 * @return true on success, false otherwise
 */
bool http_post(app_t *app, const char *url, const http_mime_t *mimes, uint32_t mimes_size, http_resp_cb_t cb,
               void *priv_data, uint32_t data_size);
