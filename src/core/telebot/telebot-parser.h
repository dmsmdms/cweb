#pragma once

#include <core/telebot/telebot-types.h>
#include <core/http/http-client.h>

/**
 * @brief Private data for POST request
 */
typedef struct {
    telebot_resp_cb_t cb; ///< Callback function to handle responce
} telebot_post_data_t;

/**
 * @brief Callback function to handle getUpdates responce
 * @param resp - [in] responce data
 */
void telebot_get_updates_cb(const chttp_resp_t *http_resp);

/**
 * @brief Callback function to handle sendMessage responce
 * @param resp - [in] responce data
 */
void telebot_send_message_cb(const chttp_resp_t *http_resp);
