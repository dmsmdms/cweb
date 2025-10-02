#pragma once

#include <bot/bot-admin.h>

/**
 * @brief Forward declaration ev_loop
 */
struct ev_loop;

/**
 * @brief Forward declaration ev_timer
 */
struct ev_timer;

/**
 * @brief Update bot admin status
 * @param loop - [in] Event loop
 * @param timer - [in] Timer watcher
 * @param events - [in] Events
 */
void bot_admin_status_upd(struct ev_loop *loop, struct ev_timer *timer, int events);

/**
 * @brief Send crypto parser status to chat
 * @param chat_id - [in] Chat ID
 * @return BOT_ADMIN_ERR_OK on success, error code otherwise
 */
bot_admin_err_t bot_admin_send_crypto_parser_status(uint64_t chat_id);
