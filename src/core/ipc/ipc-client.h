#pragma once

#include <core/base/buf.h>

/**
 * @brief IPC client error codes
 */
typedef enum {
    CIPC_ERR_OK,      ///< No error
    CIPC_ERR_CONNECT, ///< Connection error
    CIPC_ERR_NO_MEM,  ///< Out of memory
    CIPC_ERR_FAIL,    ///< General failure
    CIPC_ERR_SEND,    ///< Send failure
    CIPC_ERR_TIMEOUT, ///< Timeout error
    CIPC_ERR_MAX,
} cipc_err_t;

/**
 * @brief IPC client response structure
 */
typedef struct {
    cipc_err_t err;  ///< Error code
    buf_t body;      ///< Response data buffer
    void *user_data; ///< User data pointer
} cipc_resp_t;

/**
 * @brief IPC client response callback type
 */
typedef void (*cipc_resp_cb_t)(const cipc_resp_t *resp);

/**
 * @brief Forward declaration of IPC client structure
 */
typedef struct cipc cipc_t;

/**
 * @brief Initialize IPC client
 * @param pcipc - [out] Pointer to IPC client structure
 * @param sock_path - [in] Socket path
 * @return CIPC_ERR_OK on success, error code otherwise
 */
cipc_err_t cipc_init(cipc_t **pcipc, const char *sock_path);

/**
 * @brief Deinitialize IPC client
 * @param cipc - [in] Pointer to IPC client structure
 */
void cipc_deinit(cipc_t *cipc);

/**
 * @brief Send message to IPC server
 * @param cipc - [in] Pointer to IPC client structure
 * @param cmd - [in] Command ID
 * @param body - [in] Pointer to data buffer to be sent
 * @param cb - [in] Response callback function
 * @param user_data - [in] User private  data to be passed to the callback function
 * @return CIPC_ERR_OK on success, error code otherwise
 */
cipc_err_t cipc_send(cipc_t *cipc, uint32_t cmd, const buf_t *body, cipc_resp_cb_t cb, const buf_t *user_data);
