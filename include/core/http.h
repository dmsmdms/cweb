#pragma once

#include <common.h>
#include <curl/curl.h>

/**
 * @brief Structure to represent an HTTP response
 */
typedef struct {
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
 * @brief Instance of the HTTP multi-connection handler
 */
typedef struct {
    ev_timer timer;
    CURLM *multi;
} http_t;

/**
 * @brief Initialize the HTTP multi-connection handler
 * @param http - [out] Pointer to the HTTP handler instance
 */
void http_init(http_t *http);

/**
 * @brief Destroy the HTTP multi-connection handler and free resources
 * @param http - [in] Pointer to the HTTP handler instance
 */
void http_destroy(http_t *http);

/**
 * @brief Perform an asynchronous HTTP GET request
 * @param http - [in] Pointer to the HTTP handler instance
 * @param url - [in] URL to send the GET request to
 * @param cb - [in] Callback function to handle the HTTP response
 * @param priv_data - [in] Private data to be passed to the callback function
 */
void http_get(http_t *http, const char *url, http_resp_cb_t cb, void *priv_data);
