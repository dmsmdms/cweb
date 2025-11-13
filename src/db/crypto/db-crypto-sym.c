#include <db/crypto/db-crypto-sym.h>
#include <core/base/log.h>
#include <arpa/inet.h>
#include <string.h>
#include <lmdb.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define DB_CRYPTO_SYM_KEY      { htonl(DB_TABLE_ID_CRYPTO_SYM) }
#define DB_CRYPTO_SYM_META_KEY { htonl(DB_TABLE_ID_CRYPTO_SYM_META) }

static void db_crypto_sym_key(db_crypto_sym_key_t *key, const char *sym_name)
{
    key->table_id = htonl(DB_TABLE_ID_CRYPTO_SYM);
    strncpy(key->sym_name, sym_name, DB_CRYPTO_SYM_NAME_LEN);
}

static db_err_t db_crypto_sym_get_meta(db_crypto_sym_meta_val_t *val)
{
    db_crypto_sym_meta_key_t key = DB_CRYPTO_SYM_META_KEY;
    MDB_val mdb_val, mdb_key = DB_VAL(&key);
    db_err_t res = db_get(&mdb_key, &mdb_val);
    if(res != DB_ERR_OK) {
        if(res == DB_ERR_NOT_FOUND) {
            log_warn("crypto_sym_meta not found");
            val->last_sym_id = 1;
            return DB_ERR_OK;
        }
        return res;
    }
    db_return_if_size(mdb_val, sizeof(db_crypto_sym_meta_val_t));
    memcpy(val, mdb_val.mv_data, sizeof(db_crypto_sym_meta_val_t));
    return res;
}

static db_err_t db_crypto_sym_put_meta(db_crypto_sym_meta_val_t *val)
{
    db_crypto_sym_meta_key_t key = DB_CRYPTO_SYM_META_KEY;
    MDB_val mdb_key = DB_VAL(&key);
    MDB_val mdb_val = DB_VAL(val);
    return db_put(&mdb_key, &mdb_val);
}

db_err_t db_crypto_sym_add_global(const char *sym_name)
{
    db_crypto_sym_key_t key;
    db_crypto_sym_key(&key, sym_name);
    MDB_val mdb_val, mdb_key = DB_VAL(&key);

    // Check if the symbol already exists
    db_err_t res = db_get(&mdb_key, &mdb_val);
    if(res == DB_ERR_NOT_FOUND) {
        db_crypto_sym_meta_val_t meta;
        return_if_err(db_crypto_sym_get_meta(&meta));

        // Add the new symbol
        db_crypto_sym_val_t val = {
            .sym_id = meta.last_sym_id,
            .is_local = false,
        };
        mdb_val = DB_VAL(&val);
        return_if_err(db_put(&mdb_key, &mdb_val));

        // Update the meta info
        meta.last_sym_id++;
        return db_crypto_sym_get_meta(&meta);
    }
    return res;
}

db_err_t db_crypto_sym_del_global(const char *sym_name)
{
    db_crypto_sym_key_t key;
    db_crypto_sym_key(&key, sym_name);
    MDB_val mdb_key = DB_VAL(&key);
    return db_del(&mdb_key);
}

db_err_t db_crypto_sym_get_list(db_crypto_sym_t *arr, uint32_t arr_size, uint32_t *sym_count)
{
    db_crypto_sym_key_t key = DB_CRYPTO_SYM_KEY;
    MDB_val mdb_val, mdb_key = DB_VAL(&key);
    for(uint32_t i = 0; i < arr_size; i++) {
        db_crypto_sym_t *sym = &arr[i];
        return_if_err(db_cursor_get(&mdb_key, &mdb_val));

        db_crypto_sym_val_t *val = mdb_val.mv_data;
        db_return_if_size(mdb_val, sizeof(db_crypto_sym_val_t));
        memcpy(sym->sym_name, key.sym_name, DB_CRYPTO_SYM_NAME_LEN);
        memcpy(&sym->val, val, sizeof(db_crypto_sym_val_t));
    }
    return DB_ERR_OK;
}

db_err_t db_crypto_sym_set_local(const char *sym_name, bool is_local)
{
    db_crypto_sym_key_t key;
    db_crypto_sym_key(&key, sym_name);
    MDB_val mdb_val, mdb_key = DB_VAL(&key);
    return_if_err(db_get(&mdb_key, &mdb_val));

    // Update the entry to local
    db_crypto_sym_val_t *val = mdb_val.mv_data;
    db_return_if_size(mdb_val, sizeof(db_crypto_sym_val_t));
    val->is_local = is_local;
    return db_put(&mdb_key, &mdb_val);
}
