#pragma once

#include <core/ipc/ipc-priv.h>

/**
 * @brief IPC crypto parser command IDs
 */
typedef enum {
    IPC_CRYPTO_PARSER_CMD_GET_STATUS = IPC_CMD_MAX, ///< Get crypto parser status
    IPC_CRYPTO_PARSER_CMD_ADD_GLOBAL_SYM,           ///< Add global crypto symbol
    IPC_CRYPTO_PARSER_CMD_DEL_GLOBAL_SYM,           ///< Delete global crypto symbol
    IPC_CRYPTO_PARSER_CMD_ADD_LOCAL_SYM,            ///< Add local crypto symbol
    IPC_CRYPTO_PARSER_CMD_DEL_LOCAL_SYM,            ///< Delete local crypto symbol
    IPC_CRYPTO_PARSER_CMD_MAX,
} ipc_crypto_parser_cmd_t;

/**
 * @brief IPC crypto parser error codes
 */
typedef enum {
    IPC_CRYPTO_PARSER_ERR_DB, ///< Database error
    IPC_CRYPTO_PARSER_ERR_MAX,
} ipc_crypto_parser_err_t;

/**
 * @brief IPC crypto parser status structure
 */
typedef struct {
    uint64_t start_ts;        ///< Parser start time
    uint64_t last_upd_ts;     ///< Last update time
    uint32_t db_used_size_kb; ///< Database used size in KB
    uint32_t db_tot_size_kb;  ///< Total database size in KB
} ipc_crypto_parser_status_t;

/**
 * @brief IPC crypto parser failure response structure
 */
typedef struct {
    ipc_crypto_parser_err_t err; ///< Error code
} ipc_crypto_parser_fail_t;
