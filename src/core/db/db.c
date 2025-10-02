#include <global.h>

#define DB_FLAGS (MDB_NOTLS | MDB_NOMETASYNC | MDB_NOSYNC)

bool db_open(app_t *app)
{
    db_t *db = &app->db;
    const config_t *cfg = &app->cfg;
    int rc = mdb_env_create(&db->env);
    if(rc != MDB_SUCCESS) {
        log_error("env create failed - %s", mdb_strerror(rc));
        return false;
    }
    rc = mdb_env_set_mapsize(db->env, cfg->db_size_mb * 1024 * 1024);
    if(rc != MDB_SUCCESS) {
        log_error("env set size %uMB failed - %s", cfg->db_size_mb, mdb_strerror(rc));
        db_close(app);
        return false;
    }
    if(access(cfg->db_path, F_OK) < 0) {
        if(mkdir(cfg->db_path, 0755) < 0) {
            log_error("mkdir %s failed - %s", cfg->db_path, strerror(errno));
            db_close(app);
            return false;
        }
    }
    rc = mdb_env_open(db->env, cfg->db_path, DB_FLAGS, 0644);
    if(rc != MDB_SUCCESS) {
        log_error("env open %s failed - %s", cfg->db_path, mdb_strerror(rc));
        db_close(app);
        return false;
    }
    return true;
}

void db_close(app_t *app)
{
    db_t *db = &app->db;
    db_txn_abort(app);
    if(db->env) {
        mdb_env_sync(db->env, true);
        mdb_env_close(db->env);
        db->env = NULL;
    }
}

bool db_txn_begin(app_t *app)
{
    db_t *db = &app->db;
    if(db->txn == NULL) {
        int rc = mdb_txn_begin(db->env, NULL, 0, &db->txn);
        if(rc != MDB_SUCCESS) {
            log_error("txn begin failed - %s", mdb_strerror(rc));
            return false;
        }
    }
    if(db->dbi == 0) {
        int rc = mdb_dbi_open(db->txn, NULL, 0, &db->dbi);
        if(rc != MDB_SUCCESS) {
            log_error("dbi open failed - %s", mdb_strerror(rc));
            db_txn_abort(app);
            return false;
        }
    }
    return true;
}

bool db_cursor_get(app_t *app, const MDB_val *key, MDB_val *data, MDB_cursor_op op)
{
    db_t *db = &app->db;
    if(!db_txn_begin(app)) {
        return false;
    }
    if(db->cur == NULL) {
        int rc = mdb_cursor_open(db->txn, db->dbi, &db->cur);
        if(rc != MDB_SUCCESS) {
            log_error("cursor open failed - %s", mdb_strerror(rc));
            return false;
        }
    }
    int rc = mdb_cursor_get(db->cur, (MDB_val *)key, data, op);
    if(rc != MDB_SUCCESS) {
        if(rc != MDB_NOTFOUND) {
            log_error("cursor get failed - %s", mdb_strerror(rc));
        }
        return false;
    }
    return true;
}

bool db_txn_commit(app_t *app)
{
    db_t *db = &app->db;
    if(db->cur) {
        mdb_cursor_close(db->cur);
        db->cur = NULL;
    }
    if(db->txn) {
        int rc = mdb_txn_commit(db->txn);
        db->txn = NULL;
        if(rc != MDB_SUCCESS) {
            log_error("txn commit failed - %s", mdb_strerror(rc));
            return false;
        }
    }
    return true;
}

void db_txn_abort(app_t *app)
{
    db_t *db = &app->db;
    if(db->cur) {
        mdb_cursor_close(db->cur);
        db->cur = NULL;
    }
    if(db->txn) {
        mdb_txn_abort(db->txn);
        db->txn = NULL;
    }
}
