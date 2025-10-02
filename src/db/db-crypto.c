#include <db/db-crypto.h>
#include <db/db-crypto-table.h>
#include <core/json/json-parser.h>
#include <core/base/file.h>
#include <core/base/log.h>
#include <string.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

typedef struct {
    crypto_sym_arr_t arr;
    buf_ext_t buf;
} crypto_sym_arr_gen_t;

static json_parse_err_t json_parse_crypto_sym(const jsmntok_t *cur, const char *json, void *priv_data)
{
    crypto_sym_arr_gen_t *gen = priv_data;
    crypto_sym_t *sym = &gen->arr.data[gen->arr.count++];
    return json_parse_pstr(cur, json, &sym->name);
}

static json_parse_err_t json_parse_crypto_arr(const jsmntok_t *cur, const char *json, void *priv_data)
{
    crypto_sym_arr_gen_t *gen = priv_data;
    gen->arr.data = buf_alloc(&gen->buf, cur->size * sizeof(crypto_sym_t));
    if(gen->arr.data == NULL) {
        return JSON_PARSE_ERR_NO_MEM;
    }
    return json_parse_arr(cur, json, json_parse_crypto_sym, gen);
}

db_err_t db_crypto_init(const char *crypto_list_path)
{
    db_crypto_meta_t meta;
    db_err_t res = db_crypto_get_meta(&meta);
    if(res != DB_ERR_OK) {
        db_txn_abort();
        return res;
    }
    if(meta.sym_count > 0) {
        db_txn_abort();
        return DB_ERR_OK;
    }

    // Read default crypto list //
    char buf[CRYPTO_SYM_ARR_BUF_SIZE];
    str_t file = {
        .data = buf,
        .len = sizeof(buf),
    };
    if(file_read(crypto_list_path, &file) != FILE_ERR_OK) {
        db_txn_abort();
        return DB_ERR_OPEN;
    }

    // Parse JSON //
    char json_buf[CRYPTO_SYM_ARR_BUF_SIZE];
    crypto_sym_arr_gen_t gen = {
        .buf.data = json_buf,
        .buf.size = sizeof(json_buf),
    };
    json_item_t items[] = {
        { "global", json_parse_crypto_arr, &gen },
    };
    if(json_parse(file.data, file.len, items, ARRAY_SIZE(items)) != JSON_PARSE_ERR_OK) {
        db_txn_abort();
        return DB_ERR_PARSE;
    }

    // Store symbols in DB //
    for(uint32_t i = 0; i < gen.arr.count; i++) {
        res = db_crypto_put_sym(gen.arr.data[i].name, i + 1);
        if(res != DB_ERR_OK) {
            db_txn_abort();
            return res;
        }
    }

    // Update meta //
    meta.sym_count = gen.arr.count;
    meta.sym_id_last = gen.arr.count;
    res = db_crypto_put_meta(&meta);
    if(res != DB_ERR_OK) {
        db_txn_abort();
        return res;
    }

    return db_txn_commit();
}

db_err_t db_crypto_add(uint32_t sym_id, const crypto_t *crypto)
{
    db_crypto_t db_crypto = {
        .close = crypto->close,
        .volume = crypto->volume,
        .liq_ask = crypto->liq_ask,
        .liq_bid = crypto->liq_bid,
        .whales = crypto->whales,
    };
    db_err_t res = db_crypto_put(sym_id, crypto->ts, &db_crypto);
    if(res != DB_ERR_OK) {
        db_txn_abort();
        return res;
    }
    return db_txn_commit();
}

db_err_t db_crypto_sym_arr_get(crypto_sym_arr_t *arr, buf_ext_t *buf)
{
    db_crypto_meta_t meta;
    db_err_t res = db_crypto_get_meta(&meta);
    if(res != DB_ERR_OK) {
        db_txn_abort();
        return res;
    }
    arr->data = buf_alloc(buf, meta.sym_count * sizeof(crypto_sym_t));
    if(arr->data == NULL) {
        log_error("buf_alloc(%zu) failed", meta.sym_count * sizeof(crypto_sym_t));
        db_txn_abort();
        return DB_ERR_NO_MEM;
    }
    arr->count = meta.sym_count;

    // Retrieve symbols //
    for(uint32_t i = 0; i < meta.sym_count; i++) {
        crypto_sym_t *sym = &arr->data[i];
        uint32_t name_len;
        res = db_crypto_get_sym_next(&sym->name, &name_len, &sym->id);
        if(res != DB_ERR_OK) {
            return res;
        }
        char *name_str = buf_alloc(buf, name_len + 1);
        if(name_str == NULL) {
            log_error("buf_alloc(%u) failed", name_len + 1);
            return DB_ERR_NO_MEM;
        }
        memcpy(name_str, sym->name, name_len);
        name_str[name_len] = '\0';
        sym->name = name_str;
    }

    db_txn_abort();
    return DB_ERR_OK;
}
