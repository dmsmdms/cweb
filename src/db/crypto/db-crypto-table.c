#include <db/db-crypto-table.h>
#include <core/base/log.h>
#include <string.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define CRYPTO_SYM_TABLE "crypto_sym"
#define CRYPTO_TABLE     "crypto"

db_err_t db_crypto_get_meta(db_crypto_meta_t *meta)
{
    buf_t value = {
        .size = sizeof(db_crypto_meta_t),
    };
    db_err_t res = db_get_value_by_id_ts(CRYPTO_TABLE, 0, 0, &value);
    if(res != DB_ERR_OK) {
        if(res == DB_ERR_NOT_FOUND) {
            log_warn("crypto meta not found, initializing new meta");
            meta->sym_id_last = 0;
            meta->sym_count = 0;
            return DB_ERR_OK;
        }
        return res;
    }
    memcpy(meta, value.data, sizeof(db_crypto_meta_t));
    return DB_ERR_OK;
}

db_err_t db_crypto_put_meta(const db_crypto_meta_t *meta)
{
    buf_t value = {
        .data = (void *)meta,
        .size = sizeof(db_crypto_meta_t),
    };
    return db_put_value_by_id_ts(CRYPTO_TABLE, 0, 0, &value);
}

db_err_t db_crypto_get_sym(const char *sym_name, db_crypto_sym_t *sym)
{
    buf_t value = {
        .size = sizeof(db_crypto_sym_t),
    };
    db_err_t res = db_get_value_by_str(CRYPTO_SYM_TABLE, sym_name, &value);
    if(res != DB_ERR_OK) {
        return res;
    }
    memcpy(sym, value.data, sizeof(db_crypto_sym_t));
    return DB_ERR_OK;
}

db_err_t db_crypto_get_sym_next(const char **psym_name, uint32_t *psym_name_size, db_crypto_sym_t *sym)
{
    buf_t value = {
        .size = sizeof(db_crypto_sym_t),
    };
    db_err_t res = db_get_str_value_next(CRYPTO_SYM_TABLE, psym_name, psym_name_size, &value);
    if(res != DB_ERR_OK) {
        return res;
    }
    memcpy(sym, value.data, sizeof(db_crypto_sym_t));
    return DB_ERR_OK;
}

db_err_t db_crypto_put_sym(const char *sym_name, const db_crypto_sym_t *sym)
{
    buf_t value = {
        .data = (void *)sym,
        .size = sizeof(db_crypto_sym_t),
    };
    return db_put_value_by_str(CRYPTO_SYM_TABLE, sym_name, &value);
}

db_err_t db_crypto_get_next(uint32_t min_sym_id, uint32_t max_sym_id, uint64_t min_ts, uint64_t max_ts, uint64_t *pts,
                            db_crypto_t *crypto, db_cursor_op_t op)
{
    buf_t value = {
        .size = sizeof(db_crypto_t),
    };
    db_err_t res = db_get_ts_value_by_id_next(CRYPTO_TABLE, min_sym_id, max_sym_id, min_ts, max_ts, pts, &value, op);
    if(res != DB_ERR_OK) {
        return res;
    }
    memcpy(crypto, value.data, sizeof(db_crypto_t));
    return DB_ERR_OK;
}

db_err_t db_crypto_get_next1(uint32_t sym_id, db_cursor_op_t op, crypto_t *crypto)
{
    db_crypto_t db_crypto;
    db_err_t res = db_crypto_get_next(sym_id, sym_id, 0, UINT64_MAX, &crypto->ts, &db_crypto, op);
    if(res != DB_ERR_OK) {
        return res;
    }
    crypto->close = db_crypto.close;
    crypto->volume = db_crypto.volume;
    crypto->liq_ask = db_crypto.liq_ask;
    crypto->liq_bid = db_crypto.liq_bid;
    crypto->whales = db_crypto.whales;
    return DB_ERR_OK;
}

db_err_t db_crypto_put(uint32_t sym_id, uint64_t ts, const db_crypto_t *crypto)
{
    buf_t value = {
        .data = (void *)crypto,
        .size = sizeof(db_crypto_t),
    };
    return db_put_value_by_id_ts(CRYPTO_TABLE, sym_id, ts, &value);
}
