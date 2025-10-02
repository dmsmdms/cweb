#include <core/telebot/telebot.h>
#include <core/base/log.h>
#include <db/db-bot.h>
#include <string.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

typedef void (*telebot_ent_cb_t)(const telebot_message_entity_t *ent, const telebot_message_t *msg);

static void ent_cmd_cb(const telebot_message_entity_t *ent, const telebot_message_t *msg);

typedef struct {
    const telebot_cmd_handler_t *cmd_handlers;
    uint32_t cmd_handlers_count;
    uint32_t upd_sec;
} telebot_t;

static const telebot_ent_cb_t ent_cb_list[TELEBOT_ENTITY_TYPE_MAX] = {
    [TELEBOT_ENTITY_BOT_COMMAND] = ent_cmd_cb,
};
static telebot_t bot = { 0 };

static void ent_cmd_cb(const telebot_message_entity_t *ent, const telebot_message_t *msg)
{
    const char *cmd = &msg->text[ent->offset + 1]; // Skip '/'
    for(uint32_t i = 0; i < bot.cmd_handlers_count; i++) {
        const telebot_cmd_handler_t *handler = &bot.cmd_handlers[i];
        uint32_t cmd_len = strlen(handler->cmd);
        if((ent->length - 1) == cmd_len && memcmp(cmd, handler->cmd, cmd_len) == 0) {
            handler->cb(msg);
        }
    }
}

static void parse_entities(const telebot_update_t *update)
{
    if(update->update_type != TELEBOT_UPDATE_MESSAGE) {
        return;
    }
    const telebot_message_t *msg = &update->message;
    const telebot_message_entity_arr_t *ent_arr = &msg->entities;
    for(uint32_t i = 0; i < ent_arr->count; i++) {
        const telebot_message_entity_t *ent = &ent_arr->data[i];
        ent_cb_list[ent->type](ent, msg);
    }
}

static void get_updates_cb(const telebot_resp_t *resp)
{
    uint32_t last_upd_id;
    if(db_bot_last_upd_id_get(&last_upd_id) != DB_ERR_OK) {
        return;
    }

    if(resp) {
        if(resp->ok) {
            const telebot_update_arr_t *updates = &resp->result.updates;
            for(uint32_t i = 0; i < updates->count; i++) {
                const telebot_update_t *upd = &updates->data[i];
                parse_entities(upd);
                if(upd->update_id >= last_upd_id) {
                    last_upd_id = upd->update_id + 1;
                }
            }
            if(db_bot_last_upd_id_set(last_upd_id) != DB_ERR_OK) {
                return;
            }
        } else {
            log_error("error %d: %s", resp->result.error.code, resp->result.error.description);
        }
    }

    telebot_get_updates(last_upd_id, TELEBOT_MAX_UPDATES, bot.upd_sec, NULL, 0, get_updates_cb);
}

telebot_err_t telebot_init(const char *token, uint32_t upd_sec, const telebot_cmd_handler_t *handlers,
                           uint32_t handlers_count)
{
    if(token == NULL) {
        log_error("token is NULL");
        return TELEBOT_ERR_INVALID_ARG;
    }
    telebot_set_token(token);
    bot.upd_sec = upd_sec;
    bot.cmd_handlers = handlers;
    bot.cmd_handlers_count = handlers_count;
    get_updates_cb(NULL);
    return TELEBOT_ERR_OK;
}
