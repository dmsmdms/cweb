#include <global.h>

#define DB_FLAGS (MDB_NOSUBDIR | MDB_NOLOCK | MDB_NOTLS | MDB_NOMETASYNC | MDB_NOSYNC | MDB_WRITEMAP | MDB_MAPASYNC)

bool db_open(app_t *app)
{
    db_t *db = &app->db;
    int rc = mdb_env_create(&db->env);
    if(rc != MDB_SUCCESS) {
        log_error("env create failed - %s", mdb_strerror(rc));
        return false;
    }
    rc = mdb_env_set_mapsize(db->env, app->cfg.db_size_mb * 1024 * 1024);
    if(rc != MDB_SUCCESS) {
        log_error("env set size %uMB failed - %s", app->cfg.db_size_mb, mdb_strerror(rc));
        db_close(app);
        return false;
    }
    rc = mdb_env_open(db->env, app->cfg.db_path, DB_FLAGS, 0644);
    if(rc != MDB_SUCCESS) {
        log_error("env open %s failed - %s", app->cfg.db_path, mdb_strerror(rc));
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

bool db_txn_commit(app_t *app)
{
    db_t *db = &app->db;
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
    if(db->txn) {
        mdb_txn_abort(db->txn);
        db->txn = NULL;
    }
}
