#pragma once

#define TELEBOT_MAX_UPDATES  100
#define TELEBOT_MAX_ENTITIES 50

#include <core/http/http-client.h>

/**
 * @brief Callback function to handle getUpdates responce
 * @param resp - [in] responce data
 */
void telebot_get_updates_cb(const http_resp_t *http_resp);
