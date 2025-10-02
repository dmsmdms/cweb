#pragma once

#include <core/base/str.h>
#include <core/base/buf.h>

/**
 * @brief Forward declaration of application structure
 */
typedef enum {
    CWS_ERR_OK,        ///< No error
    CWS_ERR_CONNECT,   ///< Connection error
    CWS_ERR_CONTEXT,   ///< Context initialization error
    CWS_ERR_MEM_ALLOC, ///< Memory allocation error
    CWS_ERR_MAX,
} cws_err_t;

/**
 * @brief Structure to hold WebSocket received message information
 */
typedef struct {
    cws_err_t err;   ///< Error code of the WebSocket message
    str_t body;      ///< Body of the WebSocket message
    void *user_data; ///< User private data
} cws_recv_t;

/**
 * @brief Callback function type for handling WebSocket messages
 * @param recv - [in] Pointer to the WebSocket received message structure
 */
typedef void (*cws_recv_cb_t)(const cws_recv_t *recv);

/**
 * @brief Forward declaration of WebSocket connection structure
 */
typedef struct cws_conn cws_conn_t;

/**
 * @brief Initialize WebSocket client
 * @return CWS_ERR_OK on success, error code otherwise
 */
cws_err_t cws_init(void);

/**
 * @brief Destroy WebSocket client
 */
void cws_destroy(void);

/**
 * @brief Establish a WebSocket connection
 * @param pconn - [out] Pointer to store the WebSocket connection structure
 * @param addr - [in] WebSocket server address
 * @param path - [in] WebSocket server path
 * @param port - [in] WebSocket server port
 * @param is_ssl - [in] Whether to use SSL for the connection
 * @param cb - [in] Callback function to handle received messages
 * @param user_data - [in] User private data to be passed to the callback
 * @return CWS_ERR_OK on success, error code otherwise
 */
cws_err_t cws_connect(cws_conn_t **pconn, const char *addr, const char *path, uint32_t port, bool is_ssl,
                      cws_recv_cb_t cb, const buf_t *user_data);

/**
 * @brief Close a WebSocket connection
 * @param conn - [in] Pointer to the WebSocket connection structure
 */
void cws_disconnect(cws_conn_t *conn);
