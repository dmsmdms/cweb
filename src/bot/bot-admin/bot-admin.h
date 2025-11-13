#pragma once

#include <common.h>

/**
 * @brief Bot admin notify error codes
 */
typedef enum {
    BOT_ADMIN_ERR_OK,   ///< No error
    BOT_ADMIN_ERR_INIT, ///< Initialization error
    BOT_ADMIN_ERR_IPC,  ///< IPC error
    BOT_ADMIN_ERR_MAX,
} bot_admin_err_t;

/**
 * @brief Initialize bot admin notify module
 * @param token - [in] Telegram bot token
 * @param upd_interval_sec - [in] Update interval in seconds
 * @return BOT_ADMIN_ERR_OK on success, error code otherwise
 */
bot_admin_err_t bot_admin_init(const char *token, uint32_t upd_interval_sec);

/**
 * @brief Deinitialize bot admin notify module
 */
void bot_admin_deinit(void);
