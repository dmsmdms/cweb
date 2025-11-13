#pragma once

#include <core/telebot/telebot-types.h>

#define TELEBOT_MAX_UPDATES 100

/**
 * @brief Enumeration for telegram bot error codes
 */
typedef enum {
    TELEBOT_ERR_OK,          ///< No error
    TELEBOT_ERR_HTTP,        ///< HTTP request failed
    TELEBOT_ERR_INVALID_ARG, ///< Invalid argument
    TELEBOT_ERR_INTERNAL,    ///< Internal error
    TELEBOT_ERR_MAX,
} telebot_err_t;

/**
 * @brief This function is used to set telegram bot token
 * @param token - [in] telegram bot token
 */
void telebot_set_token(const char *token);

/**
 * @brief This function is used to get latest updates
 * @param offset - [in] Offset of the first update to be returned
 * @param limit - [in] Limit the number of updates to be retrieved. Must be between 1 and 100
 * @param timeout - [in] Timeout in seconds for long polling
 * @param allowed_updates - [in] List of the types of updates you want your bot to receive
 * @param allowed_updates_count - [in] Number of elements in allowed_updates array
 * @param cb - [in] Callback function to handle the updates
 * @return TELEBOT_ERR_OK on success, error code otherwise
 */
telebot_err_t telebot_get_updates(uint32_t offset, uint32_t limit, uint32_t timeout,
                                  const telebot_update_type_t *allowed_updates, uint32_t allowed_updates_count,
                                  telebot_resp_cb_t cb);

/**
 * @brief This function is used to send message to a chat
 * @param chat_id - [in] Unique identifier for the target chat
 * @param text - [in] Text of the message to be sent
 * @return TELEBOT_ERR_OK on success, error code otherwise
 */
telebot_err_t telebot_send_message(uint64_t chat_id, const char *text);
