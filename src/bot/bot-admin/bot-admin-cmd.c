#include <core/telebot/telebot.h>
#include <core/base/log.h>
#include <db/bot/db-bot.h>
#include <inttypes.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

static telebot_err_t start_cmd(const telebot_message_t *msg);
static telebot_err_t pass_cmd(const telebot_message_t *msg);
#ifdef CONFIG_BOT_ADMIN_CRYPTO_PARSER
static telebot_err_t crypo_parser_status_cmd(const telebot_message_t *msg);
#endif

static const telebot_cmd_handler_t cmd_handlers[] = {
    { "start", start_cmd },
    { "pass", pass_cmd },
#ifdef CONFIG_BOT_ADMIN_CRYPTO_PARSER
    { "crypo_parser_status", crypo_parser_status_cmd },
#endif
};

static telebot_err_t start_cmd(const telebot_message_t *msg)
{
    const telebot_chat_t *chat = &msg->chat;
    db_bot_admin_chat_t db_chat;
    db_err_t db_err = db_bot_admin_chat_get_by_id(chat->id, &db_chat);
    if(db_err != DB_ERR_OK) {
        if(db_err == DB_ERR_NOT_FOUND) {
            telebot_send_message(chat->id, "Use /pass <password> command to authenticate.");
            return TELEBOT_ERR_OK;
        }
        return TELEBOT_ERR_INTERNAL;
    }
    telebot_send_message(chat->id, "Hello again, admin chat!");
    return TELEBOT_ERR_OK;
}

static telebot_err_t pass_cmd(const telebot_message_t *msg)
{
    const telebot_chat_t *chat = &msg->chat;
    db_bot_admin_chat_t db_chat;
    db_err_t db_err = db_bot_admin_chat_get_by_id(chat->id, &db_chat);
    if(db_err != DB_ERR_OK) {
        if(db_err == DB_ERR_NOT_FOUND) {
            db_err = db_bot_admin_chat_new(chat->id);
            if(db_err != DB_ERR_OK) {
                return TELEBOT_ERR_INTERNAL;
            }
            telebot_send_message(chat->id, "You have been registered as an admin chat.");
            return TELEBOT_ERR_OK;
        }
        return TELEBOT_ERR_INTERNAL;
    }
    telebot_send_message(chat->id, "You are already registered as an admin chat.");
    return TELEBOT_ERR_OK;
}

#ifdef CONFIG_BOT_ADMIN_CRYPTO_PARSER
static telebot_err_t crypo_parser_status_cmd(const telebot_message_t *msg)
{
    const telebot_chat_t *chat = &msg->chat;
    return bot_admin_status_crypto_parser_send(chat->id);
}
#endif
