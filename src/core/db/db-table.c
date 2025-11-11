#include <core/db/db-table.h>
#include <core/base/log.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <endian.h>
#include <string.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

typedef struct {
    uint32_t id;
    uint32_t pad;
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

db_err_t db_get_value_by_id64(const char *table, uint64_t id, buf_t *value)
{
    uint64_t kid = htobe64(id);
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
        log_error("invalid size %s[%" PRIu64 "] got=%zu/expected=%zu", table, id, value->size, value_size);
        return DB_ERR_SIZE_MISMATCH;
    }
    return DB_ERR_OK;
}

db_err_t db_put_value_by_id64(const char *table, uint64_t id, const buf_t *value)
{
    id = htobe64(id);
    buf_t key = {
        .size = sizeof(id),
        .data = &id,
    };
    return db_put(table, &key, value);
}

db_err_t db_get_id64_value_next(const char *table, uint64_t *pid, buf_t *pvalue)
{
    buf_t key = { 0 }, value;
    db_err_t res = db_cursor_get(table, &key, &value, DB_CURSOR_OP_NEXT);
    if(res != DB_ERR_OK) {
        return res;
    }
    if(key.size != sizeof(uint64_t)) {
        log_error("invalid %s key size got=%zu/expected=%zu", table, key.size, sizeof(uint64_t));
        return DB_ERR_SIZE_MISMATCH;
    }
    memcpy(pid, key.data, sizeof(uint64_t));
    *pid = be64toh(*pid);
    if(value.size != pvalue->size) {
        log_error("invalid %s size id64=%" PRIu64 " got=%zu/expected=%zu", table, *pid, value.size, pvalue->size);
        return DB_ERR_SIZE_MISMATCH;
    }
    memcpy(pvalue->data, value.data, value.size);
    return DB_ERR_OK;
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
    db_err_t res = db_cursor_get(table, &key, &value, DB_CURSOR_OP_NEXT);
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

db_err_t db_get_value_by_str(const char *table, const char *str, buf_t *value)
{
    buf_t key = {
        .size = strlen(str),
        .data = (char *)str,
    };
    size_t value_size = value->size;
    db_err_t res = db_get(table, &key, value);
    if(res != DB_ERR_OK) {
        return res;
    }
    if(value->size != value_size) {
        log_error("invalid size %s[%s] got=%zu/expected=%zu", table, str, value->size, value_size);
        return DB_ERR_SIZE_MISMATCH;
    }
    return DB_ERR_OK;
}

db_err_t db_get_str_value_next(const char *table, const char **pstr, uint32_t *pstr_len, buf_t *value)
{
    buf_t key;
    size_t value_size = value->size;
    db_err_t res = db_cursor_get(table, &key, value, DB_CURSOR_OP_NEXT);
    if(res != DB_ERR_OK) {
        return res;
    }
    const char *str = key.data;
    if(value->size != value_size) {
        log_error("invalid size %s[%.*s] got=%zu/expected=%zu", table, (uint32_t)key.size, str, value->size,
                  value_size);
        return DB_ERR_SIZE_MISMATCH;
    }
    *pstr = str;
    *pstr_len = key.size;
    return DB_ERR_OK;
}

db_err_t db_put_value_by_str(const char *table, const char *str, const buf_t *value)
{
    buf_t key = {
        .size = strlen(str),
        .data = (char *)str,
    };
    return db_put(table, &key, value);
}

db_err_t db_get_value_by_id_ts(const char *table, uint32_t id, uint64_t ts, buf_t *value)
{
    db_key_id_ts_t key_data = {
        .id = htonl(id),
        .pad = 0,
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

db_err_t db_get_ts_value_by_id_next(const char *table, uint32_t min_id, uint32_t max_id, uint64_t min_ts,
                                    uint64_t max_ts, uint64_t *pts, buf_t *value, db_cursor_op_t op)
{
    db_key_id_ts_t kdata = {
        .id = htonl(min_id),
        .pad = 0,
        .ts = htobe64(min_ts),
    };
    buf_t key = {
        .size = sizeof(kdata),
        .data = &kdata,
    };
    size_t value_size = value->size;
    db_err_t res = db_cursor_get(table, &key, value, op);
    if(res != DB_ERR_OK) {
        return res;
    }
    if(key.size != sizeof(db_key_id_ts_t)) {
        log_error("invalid key size %s[%u:%" PRIu64 "-%u:%" PRIu64 "] got=%zu/expected=%zu", table, min_id, min_ts,
                  max_id, max_ts, key.size, sizeof(db_key_id_ts_t));
        return DB_ERR_SIZE_MISMATCH;
    }
    if(value->size != value_size) {
        log_error("invalid size %s[%u:%" PRIu64 "-%u:%" PRIu64 "] got=%zu/expected=%zu", table, min_id, min_ts, max_id,
                  max_ts, value->size, value_size);
        return DB_ERR_SIZE_MISMATCH;
    }

    memcpy(&kdata, key.data, sizeof(kdata));
    db_key_id_ts_t end_kdata = {
        .id = htonl(max_id),
        .pad = 0,
        .ts = htobe64(max_ts),
    };
    if(memcmp(&kdata, &end_kdata, sizeof(end_kdata)) >= 0) {
        return DB_ERR_NOT_FOUND;
    }
    *pts = be64toh(kdata.ts);
    return DB_ERR_OK;
}

db_err_t db_put_value_by_id_ts(const char *table, uint32_t id, uint64_t ts, const buf_t *value)
{
    db_key_id_ts_t key_data = {
        .id = htonl(id),
        .pad = 0,
        .ts = htobe64(ts),
    };
    buf_t key = {
        .size = sizeof(key_data),
        .data = &key_data,
    };
    return db_put(table, &key, value);
}
