#include <bot/bot-admin.h>
#include <bot/bot-admin-status.h>
#include <core/telebot/telebot.h>
#include <core/base/log.h>
#include <db/db-bot.h>
#include <inttypes.h>
#include <ev.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define STATUS_UPD_INTERVAL_SEC 10

typedef struct {
    ev_timer status_upd_timer;
} bot_admin_t;

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

#ifdef CONFIG_BOT_ADMIN_CRYPTO_PARSER
static void crypo_parser_status_cmd(const telebot_message_t *msg)
{
    const telebot_user_t *from = &msg->from;
    bot_admin_send_crypto_parser_status(msg->chat.id);
    log_debug("user '%s' ask crypto parser status in chat %" PRIu64, from->first_name, msg->chat.id);
}
#endif

static const telebot_cmd_handler_t cmd_handlers[] = {
    { "start", start_cmd },
#ifdef CONFIG_BOT_ADMIN_CRYPTO_PARSER
    { "crypo_parser_status", crypo_parser_status_cmd },
#endif
};
static bot_admin_t bot = { 0 };

bot_admin_err_t bot_admin_init(const char *token, uint32_t upd_interval_sec)
{
    if(telebot_init(token, upd_interval_sec, cmd_handlers, ARRAY_SIZE(cmd_handlers)) != TELEBOT_ERR_OK) {
        return BOT_ADMIN_ERR_INIT;
    }
    ev_timer_init(&bot.status_upd_timer, bot_admin_status_upd, STATUS_UPD_INTERVAL_SEC, STATUS_UPD_INTERVAL_SEC);
    ev_timer_start(EV_DEFAULT, &bot.status_upd_timer);
    return BOT_ADMIN_ERR_OK;
}

void bot_admin_deinit(void)
{
    ev_timer_stop(EV_DEFAULT, &bot.status_upd_timer);
}
