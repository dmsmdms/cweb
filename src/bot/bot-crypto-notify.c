#include <bot/bot-crypto-notify.h>
#include <core/telebot/telebot.h>
#include <core/base/log.h>
#include <core/lang.h>
#include <db/db-bot.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

static void start_cmd(const telebot_message_t *msg)
{
    const telebot_user_t *from = &msg->from;
    const telebot_chat_t *chat = &msg->chat;
    bot_user_t user = {
        .id = from->id,
        .lang_code = from->language_code,
        .first_name = from->first_name,
        .last_name = from->last_name,
        .username = from->username,
    };
    if(db_bot_user_add(&user) != DB_ERR_OK) {
        return;
    }
    bot_chat_t bot_chat = {
        .id = chat->id,
    };
    if(db_bot_chat_add(&bot_chat) != DB_ERR_OK) {
        return;
    }
    log_debug("User %d started the bot in chat %zd", from->id, chat->id);
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
