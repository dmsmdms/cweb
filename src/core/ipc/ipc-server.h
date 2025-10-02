#pragma once

#include <core/base/buf.h>

/**
 * @brief IPC server error codes
 */
typedef enum {
    SIPC_ERR_OK,      ///< No error
    SIPC_ERR_INIT,    ///< Initialization error
    SIPC_ERR_NO_MEM,  ///< Out of memory
    SIPC_ERR_INVALID, ///< Invalid argument
    SIPC_ERR_IO,      ///< I/O error
    SIPC_ERR_MAX,
} sipc_err_t;

/**
 * @brief Forward declaration of IPC connection structure
 */
typedef struct sipc_conn sipc_conn_t;

/**
 * @brief IPC server request structure
 */
typedef struct {
    sipc_conn_t *conn; ///< IPC connection
    buf_t data;        ///< Command data
    uint32_t id;       ///< Request ID
} sipc_req_t;

/**
 * @brief IPC server command callback type
 */
typedef sipc_err_t (*sipc_cmd_cb_t)(const sipc_req_t *req);

/**
 * @brief IPC server command handler structure
 */
typedef struct {
    uint32_t cmd;     ///< Command ID
    sipc_cmd_cb_t cb; ///< Command callback
} sipc_cmd_handler_t;

/**
 * @brief Initialize IPC server
 * @param sock_path - [in] Socket path
 * @param handlers - [in] Array of command handlers
 * @param handlers_count - [in] Number of command handlers
 * @return SIPC_ERR_OK on success, error code otherwise
 */
sipc_err_t sipc_init(const char *sock_path, const sipc_cmd_handler_t *handlers, uint32_t handlers_count);

/**
 * @brief Deinitialize IPC server
 */
void sipc_deinit(void);

/**
 * @brief Send IPC response
 * @param conn - [in] IPC connection
 * @param id - [in] Request ID
 * @param cmd - [in] Command ID
 * @param data - [in] Response data
 * @return SIPC_ERR_OK on success, error code otherwise
 */
sipc_err_t sipc_resp(sipc_conn_t *conn, uint32_t id, uint32_t cmd, const buf_t *data);
