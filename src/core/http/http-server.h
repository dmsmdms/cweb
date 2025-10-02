#pragma once

#include <core/base/mem.h>
#include <core/base/str.h>

/**
 * @brief Enumeration of HTTP response codes
 */
typedef enum {
    HTTP_RESP_CODE_200_OK,           ///< 200 OK
    HTTP_RESP_CODE_404_NOT_FOUND,    ///< 404 Not Found
    HTTP_RESP_CODE_500_SERVER_ERROR, ///< 500 Internal Server Error
} http_resp_code_t;

/**
 * @brief Enumeration of HTTP content types
 */
typedef enum {
    HTTP_CONTENT_TYPE_HTML, ///< text/html
    HTTP_CONTENT_TYPE_JSON, ///< application/json
} http_content_type_t;

/**
 * @brief Enumeration of HTTP connection types
 */
typedef enum {
    HTTP_CONNECTION_CLOSE,     ///< Connection closed
    HTTP_CONNECTION_KEEPALIVE, ///< Connection kept alive
} http_connection_t;

/**
 * @brief Structure to represent an HTTP server request
 */
typedef struct {
    app_t *app;       ///< Global application state structure
    const char *path; ///< Request path
} http_srv_req_t;

/**
 * @brief Structure to represent an HTTP server response
 */
typedef struct {
    str_t content;                    ///< Response content
    http_resp_code_t code;            ///< HTTP response code
    http_content_type_t content_type; ///< HTTP content type
    http_connection_t connection;     ///< HTTP connection type
} http_srv_resp_t;

/**
 * @brief Callback function type for handling HTTP server requests
 * @param req - [in] Pointer to the HTTP server request structure
 * @param resp - [out] Pointer to the HTTP server response structure to be filled
 */
typedef void (*http_srv_req_cb_t)(const http_srv_req_t *req, http_srv_resp_t *resp);

/**
 * @brief Linked list to manage multiple HTTP connections
 */
typedef LIST_HEAD(http_srv_conn_list, http_srv_conn) http_srv_conn_list_t;

/**
 * @brief HTTP server structure
 */
typedef struct {
    http_srv_conn_list_t conn_list; ///< List of connected HTTP clients
    ev_io io;                       ///< Read/Write events watcher
    mem_pool_t conns_pool;          ///< HTTP connections memory pool
} http_server_t;

/**
 * @brief HTTP connection structure
 */
typedef struct http_srv_conn {
    LIST_ENTRY(http_srv_conn) entry; ///< Linked list entry for managing multiple connections
    app_t *app;                      ///< Global application state structure
    ev_io io;                        ///< Read/Write events watcher
} http_srv_conn_t;

/**
 * @brief Initialize the HTTP server
 * @param app - [in] Pointer to the application instance
 * @return true on success, false on failure
 */
bool http_server_init(app_t *app);

/**
 * @brief Destroy the HTTP server
 * @param app - [in] Pointer to the application instance
 */
void http_server_destroy(app_t *app);
