#include <db/db-crypto-table.h>
#include <core/db/db-table.h>

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
            meta->sym_id_last = 0;
            meta->sym_count = 0;
            return DB_ERR_OK;
        }
        return res;
    }
    *meta = *(db_crypto_meta_t *)value.data;
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

db_err_t db_crypto_get_sym_next(const char **psym, uint32_t *psym_size, uint32_t *psym_id)
{
    return db_get_str_id_next(CRYPTO_SYM_TABLE, psym, psym_size, psym_id);
}

db_err_t db_crypto_put_sym(const char *sym, uint32_t sym_id)
{
    return db_put_id_by_str(CRYPTO_SYM_TABLE, sym, sym_id);
}

db_err_t db_crypto_put(uint32_t sym_id, uint64_t ts, const db_crypto_t *crypto)
{
    buf_t value = {
        .data = (void *)crypto,
        .size = sizeof(db_crypto_t),
    };
    return db_put_value_by_id_ts(CRYPTO_TABLE, sym_id, ts, &value);
}
