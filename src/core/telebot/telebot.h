#pragma once

#include <core/telebot/telebot-callback.h>
#include <core/telebot/telebot-parser.h>

/**
 * @brief Initial function to use telebot APIs
 * @param app - [in] Pointer to the application structure
 * @param bot - [out] telebot handler
 * @param token - [in] telegram bot token
 */
void telebot_init(app_t *app, telebot_t *bot, const char *token);

/**
 * @brief This function is used to get latest updates
 * @param bot - [in] telebot handler
 * @param offset - [in] Offset of the first update to be returned
 * @param limit - [in] Limit the number of updates to be retrieved. Must be between 1 and 100
 * @param timeout - [in] Timeout in seconds for long polling
 * @param allowed_updates - [in] List of the types of updates you want your bot to receive
 * @param allowed_updates_count - [in] Number of elements in allowed_updates array
 * @param cb - [in] Callback function to handle the updates
 * @return true on success, false on failure
 */
bool telebot_get_updates(telebot_t *bot, uint32_t offset, uint32_t limit, uint32_t timeout,
                         const telebot_update_type_t *allowed_updates, uint32_t allowed_updates_count,
                         telebot_resp_cb_t cb);
