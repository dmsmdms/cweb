#include <db/db-bot-table.h>
#include <core/db/db-table.h>

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

db_err_t db_bot_chat_put(uint64_t id, const db_bot_chat_t *chat)
{
    buf_t value = {
        .data = (void *)chat,
        .size = sizeof(db_bot_chat_t),
    };
    return db_put_value_by_id(BOT_CHAT_TABLE, id, &value);
}
