#include <db/db-bot-table.h>
#include <core/base/str.h>

db_err_t db_bot_user_add(const bot_user_t *user)
{
    str_buf_t buf;
    db_bot_user_t db_user;
    str_buf_init(&buf, db_user.data, sizeof(db_user.data));

    db_user.lang_code = user->lang_code;
    if(user->first_name) {
        db_user.first_name_off = buf.offset;
        buf_puts(&buf, user->first_name);
    } else {
        db_user.first_name_off = 0;
    }
    if(user->last_name) {
        db_user.last_name_off = buf.offset;
        buf_puts(&buf, user->last_name);
    } else {
        db_user.last_name_off = 0;
    }
    if(user->username) {
        db_user.username_off = buf.offset;
        buf_puts(&buf, user->username);
    } else {
        db_user.username_off = 0;
    }

    db_err_t res = db_bot_user_put(user->id, &db_user);
    if(res != DB_ERR_OK) {
        db_txn_abort();
        return res;
    }

    return db_txn_commit();
}

db_err_t db_bot_chat_add(const bot_chat_t *chat)
{
    db_bot_chat_t db_chat;
    db_chat.pad = 0;

    db_err_t res = db_bot_chat_put(chat->id, &db_chat);
    if(res != DB_ERR_OK) {
        db_txn_abort();
        return res;
    }

    return db_txn_commit();
}
