#include <db/db-bot-table.h>
#include <core/db/db-table.h>
#include <core/base/log.h>
#include <string.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define BOT_USER_TABLE "bot_user"
#define BOT_CHAT_TABLE "bot_chat"

db_err_t db_bot_user_put(uint32_t id, const db_bot_user_t *user)
{
    buf_t value = {
        .data = (void *)user,
        .size = sizeof(db_bot_user_t),
    };
    return db_put_value_by_id(BOT_USER_TABLE, id, &value);
}

db_err_t db_bot_chat_get_meta(db_bot_chat_meta_t *meta)
{
    buf_t value = {
        .size = sizeof(db_bot_chat_meta_t),
    };
    db_err_t res = db_get_value_by_id64(BOT_CHAT_TABLE, 0, &value);
    if(res != DB_ERR_OK) {
        if(res == DB_ERR_NOT_FOUND) {
            log_warn("bot chat meta not found, initializing new meta");
            meta->chat_count = 0;
            meta->last_upd_id = 0;
            return DB_ERR_OK;
        }
        return res;
    }
    memcpy(meta, value.data, sizeof(db_bot_chat_meta_t));
    return DB_ERR_OK;
}

db_err_t db_bot_chat_put_meta(const db_bot_chat_meta_t *meta)
{
    buf_t value = {
        .data = (void *)meta,
        .size = sizeof(db_bot_chat_meta_t),
    };
    return db_put_value_by_id64(BOT_CHAT_TABLE, 0, &value);
}

db_err_t db_bot_chat_put(uint64_t id, const db_bot_chat_t *chat)
{
    buf_t value = {
        .data = (void *)chat,
        .size = sizeof(db_bot_chat_t),
    };
    return db_put_value_by_id64(BOT_CHAT_TABLE, id, &value);
}

db_err_t db_bot_chat_get_next(uint64_t *pid, db_bot_chat_t *chat)
{
    buf_t value = {
        .data = chat,
        .size = sizeof(db_bot_chat_t),
    };
    while(true) {
        db_err_t res = db_get_id64_value_next(BOT_CHAT_TABLE, pid, &value);
        if(res != DB_ERR_OK) {
            return res;
        }
        if(*pid != 0) {
            break; // Ignore meta entry
        }
    }
    return DB_ERR_OK;
}
