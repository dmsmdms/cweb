#include <global.h>

static const void *db_get(app_t *app, const char *type, const void *key_data, size_t key_size)
{
    MDB_val mdb_value, mdb_key = {
        .mv_size = key_size,
        .mv_data = (void *)key_data,
    };
    if(!db_txn_begin(app)) {
        return NULL;
    }
    int rc = mdb_get(app->db.txn, app->db.dbi, &mdb_key, &mdb_value);
    if(rc != MDB_SUCCESS) {
        if(rc != MDB_NOTFOUND) {
            log_error("%s get failed - %s", type, mdb_strerror(rc));
        }
        return NULL;
    }
    return mdb_value.mv_data;
}

bool db_put(app_t *app, const char *type, const void *key_data, size_t key_size, const void *value_data,
            size_t value_size)
{
    MDB_val mdb_key = {
        .mv_size = key_size,
        .mv_data = (void *)key_data,
    };
    MDB_val mdb_value = {
        .mv_size = value_size,
        .mv_data = (void *)value_data,
    };
    if(!db_txn_begin(app)) {
        return false;
    }
    int rc = mdb_put(app->db.txn, app->db.dbi, &mdb_key, &mdb_value, 0);
    if(rc != MDB_SUCCESS) {
        log_error("%s put failed - %s", type, mdb_strerror(rc));
        return false;
    }
    return true;
}

bool db_iterate(app_t *app, const char *type, const void *key_min, const void *key_max, size_t key_size,
                db_iterate_cb_t cb, void *priv_data)
{
    MDB_val mdb_data, mdb_key = {
        .mv_size = key_size,
        .mv_data = (void *)key_min,
    };
    if(!db_cursor_get(app, &mdb_key, &mdb_data, MDB_SET_RANGE)) {
        log_error("%s iterate failed", type);
        return false;
    }
    while(true) {
        if(mdb_key.mv_size != key_size || memcmp(mdb_key.mv_data, key_max, key_size) >= 0) {
            break;
        }
        if(!cb(app, mdb_key.mv_data, mdb_data.mv_data, mdb_data.mv_size, priv_data)) {
            break;
        }
        if(!db_cursor_get(app, &mdb_key, &mdb_data, MDB_NEXT)) {
            break;
        }
    }
    db_txn_abort(app);
    return true;
}

static bool db_url_to_key(const app_t *app, db_table_t table, db_key_url_t *key, const char *url)
{
    key->table = table;
    key->url_len = strlen(url) + 1;
    if(key->url_len > sizeof(key->url)) {
        log_error("url len=%u is too long %s", key->url_len, url);
        return false;
    }
    memcpy(key->url, url, key->url_len);
    return true;
}

const void *db_get_meta(app_t *app, db_table_t table)
{
    db_key_meta_t key_meta = {
        .table = table,
    };
    return db_get(app, "meta", &key_meta, sizeof(key_meta));
}

const void *db_create_meta(app_t *app, db_table_t table, const void *def_value, size_t def_size)
{
    const void *meta = db_get_meta(app, table);
    if(meta != NULL) {
        return meta;
    }
    log_warn("meta for table=%u not found, create it", table);
    if(!db_put_meta(app, table, def_value, def_size)) {
        return NULL;
    }
    if(!db_txn_commit(app)) {
        return NULL;
    }
    return db_get_meta(app, table);
}

const void *db_get_value_by_id(app_t *app, db_table_t table, uint32_t id)
{
    db_key_id_t key_id = {
        .table = table,
        .id = id,
    };
    return db_get(app, "value by ID", &key_id, sizeof(key_id));
}

const db_key_id_t *db_get_id_by_url(app_t *app, db_table_t table, const char *url)
{
    db_key_url_t key_url;
    if(!db_url_to_key(app, table, &key_url, url)) {
        return NULL;
    }
    return db_get(app, "ID by URL", &key_url, offsetof(db_key_url_t, url) + key_url.url_len);
}

bool db_put_meta(app_t *app, db_table_t table, const void *meta, size_t size)
{
    db_key_meta_t key_meta = {
        .table = table,
    };
    return db_put(app, "meta", &key_meta, sizeof(key_meta), meta, size);
}

bool db_put_value_by_id(app_t *app, db_table_t table, uint32_t id, const void *value, size_t size)
{
    db_key_id_t key_id = {
        .table = table,
        .id = id,
    };
    return db_put(app, "value by ID", &key_id, sizeof(key_id), value, size);
}

bool db_put_id_by_url(app_t *app, db_table_t url_table, const char *url, db_table_t id_table, uint32_t id)
{
    db_key_url_t key_url;
    if(!db_url_to_key(app, url_table, &key_url, url)) {
        return false;
    }
    db_key_id_t key_id = {
        .table = id_table,
        .id = id,
    };
    return db_put(app, "ID by URL", &key_url, offsetof(db_key_url_t, url) + key_url.url_len, &key_id, sizeof(key_id));
}
