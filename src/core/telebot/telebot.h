#pragma once

#include <core/telebot/telebot-method.h>

/**
 * @brief Message command handler callback type
 * @param update - [in] pointer to telebot update structure
 */
typedef void (*telebot_msg_cb_t)(const telebot_message_t *msg);

/**
 * @brief Message command handler structure
 */
typedef struct {
    const char *cmd;     ///< Command string
    telebot_msg_cb_t cb; ///< Command handler callback
} telebot_cmd_handler_t;

/**
 * @brief Initial function to use telebot APIs
 * @param token - [in] telegram bot token
 * @param upd_interval_sec - [in] update interval in seconds
 * @param handlers - [in] array of command handlers
 * @param handlers_count - [in] number of command handlers
 * @return TELEBOT_ERR_OK on success, error code otherwise
 */
telebot_err_t telebot_init(const char *token, uint32_t upd_interval_sec, const telebot_cmd_handler_t *handlers,
                           uint32_t handers_count);
