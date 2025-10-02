#include <bot/bot-crypto-notify.h>
#include <core/telebot/telebot.h>
#include <core/base/log.h>
#include <db/db-bot.h>
#include <inttypes.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

static void start_cmd(const telebot_message_t *msg)
{
    const telebot_user_t *from = &msg->from;
    const telebot_chat_t *chat = &msg->chat;
    bot_chat_t bot_chat = {
        .id = chat->id,
    };
    if(db_bot_chat_add(&bot_chat) != DB_ERR_OK) {
        return;
    }
    log_debug("user '%s' started the chat %" PRIu64, from->first_name, chat->id);
}

static const telebot_cmd_handler_t cmd_handlers[] = {
    { "start", start_cmd },
};

bot_crypto_err_t bot_crypto_notify_init(const char *token, uint32_t upd_interval_sec)
{
    if(telebot_init(token, upd_interval_sec, cmd_handlers, ARRAY_SIZE(cmd_handlers)) != TELEBOT_ERR_OK) {
        return BOT_CRYPTO_ERR_INIT;
    }
    return BOT_CRYPTO_ERR_OK;
}
