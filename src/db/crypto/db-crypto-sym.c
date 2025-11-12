#include <db/crypto/db-crypto-sym.h>
#include <db/db-table.h>
#include <core/base/log.h>
#include <arpa/inet.h>
#include <string.h>
#include <lmdb.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

typedef struct {
    db_table_id_t table_id;
} db_crypto_sym_meta_key_t;

typedef struct {
    uint32_t last_sym_id;
    uint32_t global_sym_cnt;
    uint32_t local_sym_cnt;
} db_crypto_sym_meta_val_t;

typedef struct {
    db_table_id_t table_id;
    char sym_name[DB_CRYPTO_SYM_NAME_LEN];
} db_crypto_sym_key_t;

typedef struct {
    uint32_t sym_id;
    bool is_local;
} db_crypto_sym_val_t;

static db_err_t db_crypto_sym_get_meta(db_crypto_sym_meta_val_t *val)
{
    // Set key to get
    db_crypto_sym_meta_key_t key = {
        .table_id = htonl(DB_TABLE_ID_CRYPTO_SYM_META),
    };
    MDB_val mdb_val, mdb_key = {
        .mv_size = sizeof(db_crypto_sym_meta_key_t),
        .mv_data = &key,
    };

    // Get the meta info
    db_err_t res = db_get(&mdb_key, &mdb_val);
    if(res != DB_ERR_OK) {
        if(res == DB_ERR_NOT_FOUND) {
            log_warn("crypto_sym_meta not found");
            val->last_sym_id = 1;
            val->global_sym_cnt = 0;
            val->local_sym_cnt = 0;
            return DB_ERR_OK;
        }
        return res;
    }

    // Copy the meta info
    if(mdb_val.mv_size != sizeof(db_crypto_sym_meta_val_t)) {
        log_error("size mismatch: %zu != %zu", mdb_val.mv_size, sizeof(db_crypto_sym_meta_val_t));
        return DB_ERR_SIZE_MISMATCH;
    }
    memcpy(val, mdb_val.mv_data, sizeof(db_crypto_sym_meta_val_t));

    return res;
}

static db_err_t db_crypto_sym_put_meta(db_crypto_sym_meta_val_t *val)
{
    db_crypto_sym_meta_key_t key = {
        .table_id = htonl(DB_TABLE_ID_CRYPTO_SYM_META),
    };
    MDB_val mdb_key = {
        .mv_size = sizeof(db_crypto_sym_meta_key_t),
        .mv_data = &key,
    };
    MDB_val mdb_val = {
        .mv_size = sizeof(db_crypto_sym_meta_val_t),
        .mv_data = val,
    };
    return db_put(&mdb_key, &mdb_val);
}

db_err_t db_crypto_sym_add_global(const char *sym_name)
{
    // Set key to add
    db_crypto_sym_key_t key = {
        .table_id = htonl(DB_TABLE_ID_CRYPTO_SYM),
    };
    strncpy(key.sym_name, sym_name, DB_CRYPTO_SYM_NAME_LEN);
    MDB_val mdb_val, mdb_key = {
        .mv_size = sizeof(db_crypto_sym_key_t),
        .mv_data = &key,
    };

    // Check if entry exists
    db_err_t res = db_get(&mdb_key, &mdb_val);
    if(res == DB_ERR_NOT_FOUND) {
        // Get last sym id from meta
        db_crypto_sym_meta_val_t meta;
        res = db_crypto_sym_get_meta(&meta);
        if(res != DB_ERR_OK) {
            return res;
        }

        // Add the new entry
        db_crypto_sym_val_t val = {
            .sym_id = meta.last_sym_id,
            .is_local = false,
        };
        mdb_val.mv_size = sizeof(db_crypto_sym_val_t);
        mdb_val.mv_data = &val;
        if(db_put(&mdb_key, &mdb_val) != DB_ERR_OK) {
            return res;
        }

        // Update the meta info
        meta.last_sym_id++;
        meta.global_sym_cnt++;
        return db_crypto_sym_get_meta(&meta);
    }

    return res;
}

db_err_t db_crypto_sym_del_global(const char *sym_name)
{
    // Set key to delete
    db_crypto_sym_key_t key = {
        .table_id = htonl(DB_TABLE_ID_CRYPTO_SYM),
    };
    strncpy(key.sym_name, sym_name, DB_CRYPTO_SYM_NAME_LEN);
    MDB_val mdb_key = {
        .mv_size = sizeof(db_crypto_sym_key_t),
        .mv_data = &key,
    };

    // Delete the entry
    db_err_t res = db_del(&mdb_key);
    if(res != DB_ERR_OK) {
        return res;
    }

    // Update the meta info
    db_crypto_sym_meta_val_t meta;
    res = db_crypto_sym_get_meta(&meta);
    if(res != DB_ERR_OK) {
        return res;
    }
    meta.global_sym_cnt--;
    return db_crypto_sym_put_meta(&meta);
}

db_err_t db_crypto_sym_get_list(buf_ext_t *buf, db_crypto_sym_arr_t *arr, bool is_local)
{
    // Get the meta info
    db_crypto_sym_meta_val_t meta;
    db_err_t res = db_crypto_sym_get_meta(&meta);
    if(res != DB_ERR_OK) {
        return res;
    }

    // Allocate space for the symbols
    uint32_t cnt = is_local ? meta.local_sym_cnt : meta.global_sym_cnt;
    uint32_t tot_size = cnt * sizeof(db_crypto_sym_t);
    db_crypto_sym_t *data = buf_alloc(buf, tot_size);
    if(data == NULL) {
        log_error("buf_alloc(%u) failed", tot_size);
        return DB_ERR_NO_MEM;
    }
    arr->data = data;
    arr->count = 0;

    // Iterate through the symbols
    db_crypto_sym_key_t key = {
        .table_id = htonl(DB_TABLE_ID_CRYPTO_SYM),
    };
    MDB_val mdb_val, mdb_key = {
        .mv_size = sizeof(db_crypto_sym_key_t),
        .mv_data = &key,
    };
    for(uint32_t i = 0; i < meta.global_sym_cnt; i++) {
        res = db_cursor_get(&mdb_key, &mdb_val);
        if(res != DB_ERR_OK) {
            return res;
        }
        if(mdb_val.mv_size != sizeof(db_crypto_sym_val_t)) {
            log_error("size mismatch: %zu != %zu", mdb_val.mv_size, sizeof(db_crypto_sym_val_t));
            return DB_ERR_SIZE_MISMATCH;
        }
        db_crypto_sym_val_t *val = mdb_val.mv_data;
        if(is_local && val->is_local) {
            db_crypto_sym_t *sym = &data[arr->count];
            strncpy(sym->sym_name, key.sym_name, DB_CRYPTO_SYM_NAME_LEN);
            memcpy(&sym->sym_id, &val->sym_id, sizeof(uint32_t));
            sym->is_local = val->is_local;
            arr->count++;
        }
    }

    return DB_ERR_OK;
}

db_err_t db_crypto_sym_set_local(const char *sym_name, bool is_local)
{
    // Set key to add
    db_crypto_sym_key_t key = {
        .table_id = htonl(DB_TABLE_ID_CRYPTO_SYM),
    };
    strncpy(key.sym_name, sym_name, DB_CRYPTO_SYM_NAME_LEN);
    MDB_val mdb_val, mdb_key = {
        .mv_size = sizeof(db_crypto_sym_key_t),
        .mv_data = &key,
    };

    // Get the entry
    db_err_t res = db_get(&mdb_key, &mdb_val);
    if(res != DB_ERR_OK) {
        log_error("symbol '%s' not found", sym_name);
        return res;
    }

    // Update the entry to local
    if(mdb_val.mv_size != sizeof(db_crypto_sym_val_t)) {
        log_error("size mismatch: %zu != %zu", mdb_val.mv_size, sizeof(db_crypto_sym_val_t));
        return DB_ERR_SIZE_MISMATCH;
    }
    db_crypto_sym_val_t *val = mdb_val.mv_data;
    val->is_local = is_local;
    res = db_put(&mdb_key, &mdb_val);
    if(res != DB_ERR_OK) {
        return res;
    }

    // Update the meta info
    db_crypto_sym_meta_val_t meta;
    res = db_crypto_sym_get_meta(&meta);
    if(res != DB_ERR_OK) {
        return res;
    }
    if(is_local) {
        meta.local_sym_cnt++;
    } else {
        meta.local_sym_cnt--;
    }
    return db_crypto_sym_put_meta(&meta);
}
