#pragma once

#include <common.h>

/**
 * @brief Bot crypto notify error codes
 */
typedef enum {
    BOT_CRYPTO_ERR_OK,   ///< No error
    BOT_CRYPTO_ERR_INIT, ///< Initialization error
    BOT_CRYPTO_ERR_MAX,
} bot_crypto_err_t;

/**
 * @brief Initialize bot crypto notify module
 * @param token - [in] Telegram bot token
 * @param upd_interval_sec - [in] Update interval in seconds
 * @return BOT_CRYPTO_ERR_OK on success, error code otherwise
 */
bot_crypto_err_t bot_crypto_notify_init(const char *token, uint32_t upd_interval_sec);
