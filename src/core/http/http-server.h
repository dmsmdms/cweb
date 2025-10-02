#pragma once

#include <core/base/str.h>

/**
 * @brief Enumeration of HTTP server error codes
 */
typedef enum {
    SHTTP_ERR_OK,         ///< No error
    SHTTP_ERR_IO,         ///< I/O error
    SHTTP_ERR_SOCKET,     ///< Socket error
    SHTTP_ERR_PARAM,      ///< Invalid parameter
    SHTTP_ERR_MEM_ALLOC,  ///< Memory allocation failed
    SHTTP_ERR_NO_HANDLER, ///< No handler found for the request
    SHTTP_ERR_MAX,
} shttp_err_t;

/**
 * @brief Enumeration of HTTP server response codes
 */
typedef enum {
    SHTTP_RESP_CODE_200_OK = 200,           ///< 200 OK
    SHTTP_RESP_CODE_400_BAD_REQUEST = 400,  ///< 400 Bad Request
    SHTTP_RESP_CODE_404_NOT_FOUND = 404,    ///< 404 Not Found
    SHTTP_RESP_CODE_500_SERVER_ERROR = 500, ///< 500 Internal Server Error
    SHTTP_RESP_CODE_MAX,
} shttp_resp_code_t;

/**
 * @brief Enumeration of HTTP server content types
 */
typedef enum {
    SHTTP_CONTENT_TYPE_HTML, ///< text/html
    SHTTP_CONTENT_TYPE_JSON, ///< application/json
    SHTTP_CONTENT_TYPE_MAX,
} shttp_content_type_t;

/**
 * @brief Enumeration of HTTP server connection types
 */
typedef enum {
    SHTTP_CONNECTION_CLOSE,     ///< Connection closed
    SHTTP_CONNECTION_KEEPALIVE, ///< Connection kept alive
    SHTTP_CONNECTION_MAX,
    SHTTP_CONNECTION_DEFAULT = SHTTP_CONNECTION_KEEPALIVE,
} shttp_connection_t;

/**
 * @brief Enumeration of HTTP server methods
 */
typedef enum {
    SHTTP_METHOD_GET,  ///< HTTP GET method
    SHTTP_METHOD_POST, ///< HTTP POST method
    SHTTP_METHOD_MAX,
} shttp_method_t;

/**
 * @brief Forward declaration of the HTTP connection structure
 */
typedef struct shttp_conn shttp_conn_t;

/**
 * @brief Forward declaration of the HTTP request handler structure
 */
typedef struct shttp_req_hand shttp_req_hand_t;

/**
 * @brief Structure to represent an HTTP server request
 */
typedef struct {
    shttp_conn_t *conn;                ///< Pointer to the HTTP connection
    const char *path;                  ///< Request path
    str_t body;                        ///< Request body
    shttp_method_t method;             ///< HTTP method
    shttp_content_type_t content_type; ///< HTTP content type
} shttp_req_t;

/**
 * @brief Callback function type for handling HTTP server requests
 * @param req - [in] Pointer to the HTTP server request structure
 * @return SHTTP_ERR_OK on success, error code otherwise
 */
typedef shttp_err_t (*shttp_req_cb_t)(const shttp_req_t *req);

/**
 * @brief Initialize the HTTP server
 * @param sock_path - [in] Path to the UNIX socket
 * @return SHTTP_ERR_OK on success, error code otherwise
 */
shttp_err_t shttp_init(const char *sock_path);

/**
 * @brief Destroy the HTTP server
 * @param shttp - [in] Pointer to the HTTP server structure
 */
void shttp_destroy(void);

/**
 * @brief Add an HTTP request handler
 * @param path - [in] Request path
 * @param cb - [in] Request callback function
 * @return Pointer to the HTTP request handler structure
 */
shttp_req_hand_t *shttp_add_req_hand(const char *path, shttp_req_cb_t cb);

/**
 * @brief Delete an HTTP request handler
 * @param hand - [in] Pointer to the HTTP request handler structure
 */
void shttp_del_req_hand(shttp_req_hand_t *hand);

/**
 * @brief Send an HTTP response
 * @param conn - [in] Pointer to the HTTP connection
 * @param code - [in] HTTP response code
 * @param content_type - [in] HTTP content type
 * @param connection - [in] HTTP connection type
 * @param body - [in] Pointer to the response body string
 * @return SHTTP_ERR_OK on success, error code otherwise
 */
shttp_err_t shttp_resp(shttp_conn_t *conn, shttp_resp_code_t code, shttp_content_type_t content_type,
                       shttp_connection_t connection, const str_t *body);

/**
 * @brief Free the HTTP connection buffer
 * @param conn - [in] Pointer to the HTTP connection
 */
void shttp_buf_free(shttp_conn_t *conn);
