#pragma once

#include <ipc/ipc-crypto-parser.h>
#include <core/ipc/ipc-client.h>

/**
 * @brief Initialize IPC crypto parser client
 * @param sock_path - [in] Socket path
 * @return CIPC_ERR_OK on success, error code otherwise
 */
cipc_err_t cipc_crypto_parser_init(const char *sock_path);

/**
 * @brief Deinitialize IPC crypto parser client
 */
void cipc_crypto_parser_deinit(void);

/**
 * @brief Get crypto parser status
 * @param cb - [in] Response callback
 * @param user_data - [in] User data passed to callback
 * @return CIPC_ERR_OK on success, error code otherwise
 */
cipc_err_t cipc_crypto_parser_get_status(cipc_resp_cb_t cb, const buf_t *user_data);
