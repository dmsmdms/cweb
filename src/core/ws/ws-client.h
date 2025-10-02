#pragma once

#include <core/base/mem.h>
#include <libwebsockets.h>

/**
 * @brief Structure to hold WebSocket received message information
 */
typedef struct {
    app_t *app;       ///< Global application state structure
    const char *data; ///< Received data
    uint32_t size;    ///< Size of the received data
    void *priv_data;  ///< Private data associated with the request
} ws_recv_t;

/**
 * @brief Callback function type for handling WebSocket messages
 * @param recv - [in] Pointer to the WebSocket received message structure
 */
typedef void (*ws_recv_cb_t)(const ws_recv_t *recv);

/**
 * @brief Structure representing a WebSocket connection
 */
typedef struct {
    struct lws *wsi;      ///< Pointer to the libwebsockets connection instance
    ev_timer timer;       ///< Timer for reconnection attempts
    app_t *app;           ///< Pointer to the application instance
    ws_recv_cb_t recv_cb; ///< Callback function to handle the messages
    void *priv_data;      ///< Private data associated with the connection
    uint32_t port;        ///< WebSocket server port
    char path[128 * 48];  ///< WebSocket server path
    char addr[64];        ///< WebSocket server address
    bool is_ssl;          ///< Whether the connection uses SSL
    bool is_active;       ///< Whether the connection not closed by user
    bool need_reconnect;  ///< Whether to attempt reconnection on disconnection
} ws_conn_t;

/**
 * @brief WebSocket client structure
 */
typedef struct {
    struct lws_context *context; ///< Pointer to the libwebsockets context
    mem_pool_t conn_pool;        ///< Memory pool for managing WebSocket connections
} ws_client_t;

/**
 * @brief Initialize WebSocket client
 * @param app - [in] Pointer to the application instance
 * @return True on success, false on failure
 */
bool ws_client_init(app_t *app);

/**
 * @brief Destroy WebSocket client
 * @param app - [in] Pointer to the application instance
 */
void ws_client_destroy(app_t *app);

/**
 * @brief Establish a WebSocket connection
 * @param app - [in] Pointer to the application instance
 * @param addr - [in] WebSocket server address
 * @param path - [in] WebSocket server path
 * @param port - [in] WebSocket server port
 * @param is_ssl - [in] Whether to use SSL for the connection
 * @param cb - [in] Callback function to handle received messages
 * @param priv_data - [in] Private data associated with the connection
 * @return Pointer to the WebSocket connection structure, or NULL on failure
 */
ws_conn_t *ws_connect(app_t *app, const char *addr, const char *path, uint32_t port, bool is_ssl, ws_recv_cb_t cb,
                      void *priv_data);

/**
 * @brief Close a WebSocket connection
 * @param conn - [in] Pointer to the WebSocket connection structure
 */
void ws_disconnect(ws_conn_t *conn);
