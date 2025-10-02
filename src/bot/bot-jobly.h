#pragma once

#include <core/telebot/telebot.h>

/**
 * @brief Jobly Telegram bot structure
 */
typedef struct {
    telebot_t bot; ///< Telegram bot handler
} bot_jobly_t;

/**
 * @brief Initialize the jobly Telegram bot
 * @param app - [in] Pointer to the application structure
 * @return true on success, false on failure
 */
bool bot_jobly_init(app_t *app);
