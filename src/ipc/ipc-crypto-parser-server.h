#pragma once

#include <ipc/ipc-crypto-parser.h>
#include <core/ipc/ipc-server.h>

/**
 * @brief Initialize IPC crypto parser server
 * @param sock_path - [in] Socket path
 * @return SIPC_ERR_OK on success, error code otherwise
 */
sipc_err_t sipc_crypto_parser_init(const char *sock_path);
