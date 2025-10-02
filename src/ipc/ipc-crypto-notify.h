#pragma once

#include <core/ipc/ipc-priv.h>

/**
 * @brief IPC crypto notify command IDs
 */
typedef enum {
    IPC_CRYPTO_NOTIFY_CMD_INFO = IPC_CMD_MAX, ///< Crypto notify info
    IPC_CRYPTO_NOTIFY_CMD_MAX,
} ipc_crypto_notify_cmd_t;

/**
 * @brief IPC crypto notify types
 */
typedef enum PACKED {
    IPC_CRYPTO_NOTIFY_TYPE_PUMP, ///< Crypto notify pump
    IPC_CRYPTO_NOTIFY_TYPE_DUMP, ///< Crypto notify dump
    IPC_CRYPTO_NOTIFY_TYPE_MAX,
} ipc_crypto_notify_type_t;

/**
 * @brief IPC crypto notify status structure
 */
typedef struct {
    ipc_crypto_notify_type_t type; ///< Notify type
    uint8_t ai_score;              ///< AI score
    float price;                   ///< Price
    float rsi;                     ///< Relative Strength Index
    float volume;                  ///< Volume
    float slope;                   ///< Slope
    float liq_bid;                 ///< Liquidation bid
    float liq_ask;                 ///< Liquidation ask
    float bid_ask_ratio;           ///< Bid-ask ratio
} ipc_crypto_notify_info_t;
