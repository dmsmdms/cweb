#include <core/db/db.h>
#include <core/base/log.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <lmdb.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define DB_FLAGS (MDB_NOTLS | MDB_NOMETASYNC | MDB_NOSYNC)

typedef struct {
    MDB_env *env;
    MDB_txn *txn;
    MDB_cursor *cur;
    MDB_dbi dbi;
} db_t;

static db_t db = { 0 };

STATIC_ASSERT((uint32_t)MDB_SET_RANGE == DB_CURSOR_OP_SET_RANGE);
STATIC_ASSERT((uint32_t)MDB_NEXT == DB_CURSOR_OP_NEXT);

db_err_t db_open(const char *path, uint32_t size_mb, uint32_t max_dbs, bool rd_only)
{
    int rc = mdb_env_create(&db.env);
    if(rc != MDB_SUCCESS) {
        log_error("env create failed - %s", mdb_strerror(rc));
        db_close();
        return DB_ERR_ENV_CREATE;
    }
    rc = mdb_env_set_mapsize(db.env, (size_t)size_mb * 1024 * 1024);
    if(rc != MDB_SUCCESS) {
        log_error("env set size %uMB failed - %s", size_mb, mdb_strerror(rc));
        db_close();
        return DB_ERR_ENV_CREATE;
    }

    rc = mdb_env_set_maxdbs(db.env, max_dbs);
    if(rc != MDB_SUCCESS) {
        log_error("env set max dbs %u failed - %s", max_dbs, mdb_strerror(rc));
        db_close();
        return DB_ERR_ENV_CREATE;
    }

    if(access(path, F_OK) < 0) {
        if(mkdir(path, 0755) < 0) {
            log_error("mkdir %s failed - %s", path, strerror(errno));
            db_close();
            return DB_ERR_OPEN;
        }
    }

    uint32_t flags = DB_FLAGS;
    if(rd_only) {
        flags |= MDB_RDONLY;
    }
    rc = mdb_env_open(db.env, path, flags, 0644);
    if(rc != MDB_SUCCESS) {
        log_error("env open %s failed - %s", path, mdb_strerror(rc));
        db_close();
        return DB_ERR_OPEN;
    }

    return DB_ERR_OK;
}

db_err_t db_get_stat(db_stat_t *stat)
{
    MDB_envinfo mdb_info;
    int rc = mdb_env_info(db.env, &mdb_info);
    if(rc != MDB_SUCCESS) {
        log_error("info failed - %s", mdb_strerror(rc));
        return DB_ERR_ENV_INFO;
    }

    MDB_stat mdb_stat;
    rc = mdb_env_stat(db.env, &mdb_stat);
    if(rc != MDB_SUCCESS) {
        log_error("stat failed - %s", mdb_strerror(rc));
        return DB_ERR_ENV_INFO;
    }

    stat->tot_size = mdb_info.me_mapsize;
    stat->used_size = (mdb_info.me_last_pgno + 1) * mdb_stat.ms_psize;

    return DB_ERR_OK;
}

db_err_t db_sync(void)
{
    int rc = mdb_env_sync(db.env, true);
    if(rc != MDB_SUCCESS) {
        log_error("sync failed - %s", mdb_strerror(rc));
        return DB_ERR_SYNC;
    }
    return DB_ERR_OK;
}

void db_close(void)
{
    db_txn_abort();
    if(db.env) {
        mdb_env_sync(db.env, true);
        mdb_env_close(db.env);
        db.env = NULL;
    }
}

db_err_t db_txn_begin(bool rd_only)
{
    if(db.txn == NULL) {
        uint32_t flags = rd_only ? MDB_RDONLY : 0;
        int rc = mdb_txn_begin(db.env, NULL, flags, &db.txn);
        if(rc != MDB_SUCCESS) {
            log_error("txn begin failed - %s", mdb_strerror(rc));
            return DB_ERR_TXN_BEGIN;
        }
    }
    return DB_ERR_OK;
}

static db_err_t db_name_open(const char *db_name, bool rd_only)
{
    db_err_t res = db_txn_begin(rd_only);
    if(res != DB_ERR_OK) {
        return res;
    }
    int rc = mdb_dbi_open(db.txn, db_name, MDB_CREATE, &db.dbi);
    if(rc != MDB_SUCCESS) {
        log_error("dbi %s open failed - %s", db_name, mdb_strerror(rc));
        return DB_ERR_DBI_OPEN;
    }
    return DB_ERR_OK;
}

db_err_t db_txn_commit(void)
{
    if(db.cur) {
        mdb_cursor_close(db.cur);
        db.cur = NULL;
    }
    if(db.txn) {
        int rc = mdb_txn_commit(db.txn);
        if(rc != MDB_SUCCESS) {
            log_error("txn commit failed - %s", mdb_strerror(rc));
            return DB_ERR_TXN_COMMIT;
        }
        db.txn = NULL;
    }
    return DB_ERR_OK;
}

void db_txn_abort(void)
{
    if(db.cur) {
        mdb_cursor_close(db.cur);
        db.cur = NULL;
    }
    if(db.txn) {
        mdb_txn_abort(db.txn);
        db.txn = NULL;
    }
}

db_err_t db_get(MDB_val *key, MDB_val *val)
{
    db_err_t res = db_name_open(db_name, true);
    if(res != DB_ERR_OK) {
        return res;
    }
    MDB_val mdb_value, mdb_key = {
        .mv_size = key->size,
        .mv_data = key->data,
    };
    int rc = mdb_get(db.txn, db.dbi, &mdb_key, &mdb_value);
    if(rc != MDB_SUCCESS) {
        if(rc != MDB_NOTFOUND) {
            log_error("db %s get failed - %s", db_name, mdb_strerror(rc));
            return DB_ERR_DBI_GET;
        }
        return DB_ERR_NOT_FOUND;
    }
    value->size = mdb_value.mv_size;
    value->data = mdb_value.mv_data;
    return DB_ERR_OK;
}

db_err_t db_put(const buf_t *key, const buf_t *value)
{
    db_err_t res = db_name_open(db_name, false);
    if(res != DB_ERR_OK) {
        return res;
    }
    MDB_val mdb_key = {
        .mv_size = key->size,
        .mv_data = key->data,
    };
    MDB_val mdb_value = {
        .mv_size = value->size,
        .mv_data = value->data,
    };
    int rc = mdb_put(db.txn, db.dbi, &mdb_key, &mdb_value, 0);
    if(rc != MDB_SUCCESS) {
        log_error("db %s put failed - %s", db_name, mdb_strerror(rc));
        return DB_ERR_DBI_PUT;
    }
    return DB_ERR_OK;
}

db_err_t db_cursor_get(const char *db_name, buf_t *key, buf_t *value, db_cursor_op_t op)
{
    db_err_t res = db_name_open(db_name, true);
    if(res != DB_ERR_OK) {
        return res;
    }
    if(db.cur && mdb_cursor_dbi(db.cur) != db.dbi) {
        mdb_cursor_close(db.cur);
        db.cur = NULL;
    }
    if(db.cur == NULL) {
        int rc = mdb_cursor_open(db.txn, db.dbi, &db.cur);
        if(rc != MDB_SUCCESS) {
            log_error("db %s cursor open failed - %s", db_name, mdb_strerror(rc));
            return DB_ERR_DBI_GET;
        }
    }
    MDB_val mdb_value, mdb_key = {
        .mv_size = key->size,
        .mv_data = key->data,
    };
    int rc = mdb_cursor_get(db.cur, &mdb_key, &mdb_value, (uint32_t)op);
    if(rc != MDB_SUCCESS) {
        if(rc != MDB_NOTFOUND) {
            log_error("db %s cursor get failed - %s", db_name, mdb_strerror(rc));
            return DB_ERR_DBI_GET;
        }
        return DB_ERR_NOT_FOUND;
    }
    key->size = mdb_key.mv_size;
    key->data = mdb_key.mv_data;
    value->size = mdb_value.mv_size;
    value->data = mdb_value.mv_data;
    return DB_ERR_OK;
}
