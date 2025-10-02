#include <db/db-bot-table.h>
#include <core/base/log.h>
#include <core/base/str.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

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
    db_bot_chat_t db_chat = {
        .pad = 0,
    };
    db_bot_chat_meta_t meta;
    db_err_t res = db_bot_chat_get_meta(&meta);
    if(res != DB_ERR_OK) {
        db_txn_abort();
        return res;
    }
    res = db_bot_chat_put(chat->id, &db_chat);
    if(res != DB_ERR_OK) {
        db_txn_abort();
        return res;
    }
    meta.chat_count++;
    res = db_bot_chat_put_meta(&meta);
    if(res != DB_ERR_OK) {
        db_txn_abort();
        return res;
    }

    return db_txn_commit();
}

db_err_t db_bot_last_upd_id_get(uint32_t *last_upd_id)
{
    db_bot_chat_meta_t meta;
    db_err_t res = db_bot_chat_get_meta(&meta);
    db_txn_abort();
    if(res != DB_ERR_OK) {
        return res;
    }
    *last_upd_id = meta.last_upd_id;
    return DB_ERR_OK;
}

db_err_t db_bot_last_upd_id_set(uint32_t last_upd_id)
{
    db_bot_chat_meta_t meta;
    db_err_t res = db_bot_chat_get_meta(&meta);
    if(res != DB_ERR_OK) {
        db_txn_abort();
        return res;
    }
    if(meta.last_upd_id == last_upd_id) {
        db_txn_abort();
        return DB_ERR_OK;
    }

    meta.last_upd_id = last_upd_id;
    res = db_bot_chat_put_meta(&meta);
    if(res != DB_ERR_OK) {
        db_txn_abort();
        return res;
    }

    return db_txn_commit();
}

db_err_t db_bot_chat_arr_get(bot_chat_arr_t *arr, buf_ext_t *buf)
{
    db_bot_chat_meta_t meta;
    db_err_t res = db_bot_chat_get_meta(&meta);
    if(res != DB_ERR_OK) {
        db_txn_abort();
        return res;
    }
    uint32_t tot_size = meta.chat_count * sizeof(bot_chat_t);
    arr->data = buf_alloc(buf, tot_size);
    if(arr->data == NULL) {
        log_error("buf_alloc(%u) failed", tot_size);
        db_txn_abort();
        return DB_ERR_NO_MEM;
    }
    arr->count = 0;

    // Retrieve chats //
    while(arr->count < meta.chat_count) {
        uint64_t id;
        db_bot_chat_t db_chat;
        res = db_bot_chat_get_next(&id, &db_chat);
        if(res != DB_ERR_OK) {
            return res;
        }
        bot_chat_t *chat = &arr->data[arr->count];
        chat->id = id;
        arr->count++;
    }

    db_txn_abort();
    return DB_ERR_OK;
}
