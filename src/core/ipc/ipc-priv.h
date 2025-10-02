#pragma once

#include <common.h>

#define IPC_BUF_SIZE (64 * 1024)

/**
 * @brief IPC command result
 */
typedef enum {
    IPC_CMD_OK,       ///< Command completed successfully
    IPC_CMD_FAIL,     ///< Command failed
    IPC_CMD_FAIL_SRV, ///< Command failed on server side
    IPC_CMD_MAX,
} ipc_cmd_t;

/**
 * @brief IPC message header
 */
typedef struct {
    uint16_t cmd; ///< Command identifier
    uint16_t id;  ///< Message ID
    uint32_t len; ///< Length of the payload
    char data[];  ///< Payload data
} ipc_header_t;
