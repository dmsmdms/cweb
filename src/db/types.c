#include <global.h>

static const void *db_get(const db_t *db, const char *type, const void *key_data, size_t key_size)
{
    app_t *app = container_of(db, app_t, db);
    MDB_val mdb_value, mdb_key = {
        .mv_size = key_size,
        .mv_data = (void *)key_data,
    };
    int rc = mdb_get(app->db.txn, app->db.dbi, &mdb_key, &mdb_value);
    if(rc != MDB_SUCCESS) {
        if(rc != MDB_NOTFOUND) {
            log_error("%s get failed: %s", type, mdb_strerror(rc));
        }
        return NULL;
    }
    return mdb_value.mv_data;
}

static bool db_put(const db_t *db, const char *type, const void *key_data, size_t key_size, const void *value_data,
                   size_t value_size)
{
    app_t *app = container_of(db, app_t, db);
    MDB_val mdb_key = {
        .mv_size = key_size,
        .mv_data = (void *)key_data,
    };
    MDB_val mdb_value = {
        .mv_size = value_size,
        .mv_data = (void *)value_data,
    };
    int rc = mdb_put(app->db.txn, app->db.dbi, &mdb_key, &mdb_value, 0);
    if(rc != MDB_SUCCESS) {
        log_error("%s put failed: %s", type, mdb_strerror(rc));
        return false;
    }
    return true;
}

static bool db_url_to_key(const db_t *db, db_table_t table, db_key_url_t *key, const char *url)
{
    app_t *app = container_of(db, app_t, db);
    key->table = table;
    key->url_len = strlen(url) + 1;
    if(key->url_len > sizeof(key->url)) {
        log_error("url len=%u is too long %s", key->url_len, url);
        return false;
    }
    memcpy(key->url, url, key->url_len);
    return true;
}

const void *db_get_meta(const db_t *db, db_table_t table)
{
    db_key_meta_t key_meta = {
        .table = table,
    };
    return db_get(db, "meta", &key_meta, sizeof(key_meta));
}

const void *db_get_value_by_id(const db_t *db, db_table_t table, uint32_t id)
{
    db_key_id_t key_id = {
        .table = table,
        .id = id,
    };
    return db_get(db, "value by ID", &key_id, sizeof(key_id));
}

const db_key_id_t *db_get_id_by_url(const db_t *db, db_table_t table, const char *url)
{
    db_key_url_t key_url;
    if(db_url_to_key(db, table, &key_url, url) == false) {
        return NULL;
    }
    return db_get(db, "ID by URL", &key_url, offsetof(db_key_url_t, url) + key_url.url_len);
}

bool db_put_meta(const db_t *db, db_table_t table, const void *meta, size_t size)
{
    db_key_meta_t key_meta = {
        .table = table,
    };
    return db_put(db, "meta", &key_meta, sizeof(key_meta), meta, size);
}

bool db_put_value_by_id(db_t *db, db_table_t table, uint32_t id, const void *value, size_t size)
{
    db_key_id_t key_id = {
        .table = table,
        .id = id,
    };
    return db_put(db, "value by ID", &key_id, sizeof(key_id), value, size);
}

bool db_put_id_by_url(db_t *db, db_table_t url_table, const char *url, db_table_t id_table, uint32_t id)
{
    db_key_url_t key_url;
    if(db_url_to_key(db, url_table, &key_url, url) == false) {
        return false;
    }
    db_key_id_t key_id = {
        .table = id_table,
        .id = id,
    };
    return db_put(db, "ID by URL", &key_url, offsetof(db_key_url_t, url) + key_url.url_len, &key_id, sizeof(key_id));
}
