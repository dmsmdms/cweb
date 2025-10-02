#include <core/db/db-table.h>
#include <core/base/log.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <string.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

typedef struct {
    uint32_t id;
    uint64_t ts;
} db_key_id_ts_t;

db_err_t db_get_value_by_id(const char *table, uint32_t id, buf_t *value)
{
    uint32_t kid = htonl(id);
    buf_t key = {
        .size = sizeof(kid),
        .data = &kid,
    };
    size_t value_size = value->size;
    db_err_t res = db_get(table, &key, value);
    if(res != DB_ERR_OK) {
        return res;
    }
    if(value->size != value_size) {
        log_error("invalid size %s[%u] got=%zu/expected=%zu", table, id, value->size, value_size);
        return DB_ERR_SIZE_MISMATCH;
    }
    return DB_ERR_OK;
}

db_err_t db_put_value_by_id(const char *table, uint32_t id, const buf_t *value)
{
    id = htonl(id);
    buf_t key = {
        .size = sizeof(id),
        .data = &id,
    };
    return db_put(table, &key, value);
}

db_err_t db_get_id_by_str(const char *table, const char *str, uint32_t *pid)
{
    buf_t key = {
        .size = strlen(str),
        .data = (char *)str,
    };
    buf_t value;
    db_err_t res = db_get(table, &key, &value);
    if(res != DB_ERR_OK) {
        return res;
    }
    if(value.size != sizeof(uint32_t)) {
        log_error("invalid size %s[%s] got=%zu/expected=%zu", table, str, value.size, sizeof(uint32_t));
        return DB_ERR_SIZE_MISMATCH;
    }
    memcpy(pid, value.data, sizeof(uint32_t));
    return DB_ERR_OK;
}

db_err_t db_get_str_id_next(const char *table, const char **pstr, uint32_t *pstr_len, uint32_t *pid)
{
    buf_t key, value;
    db_err_t res = db_cursor_get(table, &key, &value);
    if(res != DB_ERR_OK) {
        return res;
    }
    const char *str = key.data;
    if(value.size != sizeof(uint32_t)) {
        log_error("invalid size %s[%.*s] got=%zu/expected=%zu", table, (uint32_t)key.size, str, value.size,
                  sizeof(uint32_t));
        return DB_ERR_SIZE_MISMATCH;
    }
    *pstr = str;
    *pstr_len = key.size;
    memcpy(pid, value.data, sizeof(uint32_t));
    return DB_ERR_OK;
}

db_err_t db_put_id_by_str(const char *table, const char *str, uint32_t id)
{
    buf_t key = {
        .size = strlen(str),
        .data = (char *)str,
    };
    buf_t value = {
        .size = sizeof(id),
        .data = &id,
    };
    return db_put(table, &key, &value);
}

db_err_t db_get_value_by_id_ts(const char *table, uint32_t id, uint64_t ts, buf_t *value)
{
    db_key_id_ts_t key_data = {
        .id = htonl(id),
        .ts = htobe64(ts),
    };
    buf_t key = {
        .size = sizeof(key_data),
        .data = &key_data,
    };
    size_t value_size = value->size;
    db_err_t res = db_get(table, &key, value);
    if(res != DB_ERR_OK) {
        return res;
    }
    if(value->size != value_size) {
        log_error("invalid size %s[%u:%" PRIu64 "] got=%zu/expected=%zu", table, id, ts, value->size, value_size);
        return DB_ERR_SIZE_MISMATCH;
    }
    return DB_ERR_OK;
}

db_err_t db_put_value_by_id_ts(const char *table, uint32_t id, uint64_t ts, const buf_t *value)
{
    db_key_id_ts_t key_data = {
        .id = htonl(id),
        .ts = htobe64(ts),
    };
    buf_t key = {
        .size = sizeof(key_data),
        .data = &key_data,
    };
    return db_put(table, &key, value);
}
