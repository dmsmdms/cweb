#include <global.h>

#define DB_TYPE "crypto"

typedef struct {
    crypto_arr_t *arr;
    uint32_t index;
    uint32_t interval;
} crypto_iterate_priv_t;

static bool db_crypto_put_meta(app_t *app, const crypto_sym_t *syms)
{
    db_crypto_meta_t meta;
    uint32_t offset = syms->count * sizeof(uint16_t);
    uint16_t *offsets = (uint16_t *)meta.data;
    meta.sumbol_count = syms->count;
    for(uint32_t i = 0; i < syms->count; i++) {
        const char *sym = syms->symbols[i];
        uint32_t len = strlen(sym) + 1;
        if(offset + len > sizeof(meta.data)) {
            log_error("crypto symbols too large");
            return false;
        }
        memcpy(meta.data + offset, sym, len);
        offsets[i] = offset;
        offset += len;
    }
    if(!db_put_meta(app, DB_TABLE_CRYPTO, &meta, offsetof(db_crypto_meta_t, data) + offset)) {
        return false;
    }
    return db_txn_commit(app);
}

static bool parse_global(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    crypto_sym_t *syms = priv_data;
    if(syms->count >= CRYPTO_MAX_SYMBOLS) {
        log_error("too many crypto symbols");
        return false;
    }
    return json_parse_pstr(app, cur, json, &syms->symbols[syms->count++]);
}

static bool parse_global_arr(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    return json_parse_arr(app, cur, json, parse_global, priv_data);
}

static bool crypto_list_parse_cb(app_t *app, const char *data, uint32_t size, void *priv_data)
{
    UNUSED(priv_data);
    crypto_sym_t syms;
    syms.count = 0;
    json_item_t items[] = {
        { "global", parse_global_arr, &syms },
    };
    if(!json_parse_wrap(app, data, size, items, ARRAY_SIZE(items))) {
        return false;
    }
    return db_crypto_put_meta(app, &syms);
}

static const db_crypto_meta_t *db_crypto_get_meta(app_t *app)
{
    const db_crypto_meta_t *meta = db_get_meta(app, DB_TABLE_CRYPTO);
    if(meta == NULL) {
        log_warn("meta for DB_TABLE_CRYPTO not found, create it");
        const config_t *cfg = &app->cfg;
        uint32_t mem_offset = mem_get_offset(app);
        if(file_read(app, cfg->crypto_list_path, crypto_list_parse_cb, NULL)) {
            meta = db_get_meta(app, DB_TABLE_CRYPTO);
        } else {
            log_error("read crypto list %s failed", cfg->crypto_list_path);
        }
        mem_put_offset(app, mem_offset);
    }
    return meta;
}

static bool db_crypto_to_key(const app_t *app, db_key_crypto_t *key, const char *symbol, uint64_t timestamp)
{
    bzero(key, sizeof(db_key_crypto_t));
    key->table = DB_TABLE_CRYPTO;
    key->timestamp = timestamp;
    uint32_t symbol_len = strlen(symbol) + 1;
    if(symbol_len > sizeof(key->symbol)) {
        log_error("symbol len=%u is too long %s", symbol_len, symbol);
        return false;
    }
    memcpy(key->symbol, symbol, symbol_len);
    return true;
}

static bool export_csv_cb(app_t *app, str_t *out, const MDB_val *mdb_key, const MDB_val *mdb_value, void *priv_data)
{
    UNUSED(priv_data);
    switch(mdb_key->mv_size) {
    case sizeof(db_key_crypto_t): {
        const db_key_crypto_t *key = mdb_key->mv_data;
        const db_crypto_t *crypto = mdb_value->mv_data;
        size_t n = snprintf(out->data, out->len, "%s,%" PRIu64 ",%g,%g,%g,%g,%hhu\n", key->symbol, key->timestamp,
                            crypto->close_price, crypto->volume, crypto->liq_ask, crypto->liq_bid, crypto->whales);
        out->data += n;
        out->len -= n;
    } break;
    case sizeof(db_key_meta_t): {
        size_t n = snprintf(out->data, out->len, "symbol,timestamp,close_price,volume,liq_ask,liq_bid,whales\n");
        out->data += n;
        out->len -= n;
    } break;
    default:
        log_error("unknown key_size=%zu", mdb_key->mv_size);
        return false;
    }
    return true;
}

bool db_crypto_export_csv(app_t *app, str_t *out, MDB_val *prev_key)
{
    return db_export(app, DB_TYPE, out, export_csv_cb, prev_key, NULL);
}

static bool export_csv_file_cb(app_t *app, str_t *chunk, void *priv_data)
{
    MDB_val *prev_key = priv_data;
    bool res = db_crypto_export_csv(app, chunk, prev_key);
    return res && (prev_key->mv_data != NULL);
}

bool db_crypto_export_csv_file(app_t *app, const char *path)
{
    MDB_val prev_key = { 0 };
    return file_stream(app, path, export_csv_file_cb, &prev_key);
}

static bool db_crypto_iterate_cb(app_t *app, const void *key_data, const void *value_data, size_t value_size,
                                 void *priv_data)

{
    UNUSED(app);
    UNUSED(value_size);
    crypto_iterate_priv_t *priv = priv_data;
    crypto_arr_t *arr = priv->arr;
    if(priv->index >= arr->count) {
        return false;
    }
    const db_key_crypto_t *key = key_data;
    if(priv->index > 0) {
        const crypto_t *prev_key = &arr->data[priv->index - 1];
        if(key->timestamp < prev_key->timestamp + priv->interval) {
            return true;
        }
    }
    const db_crypto_t *db_crypto = value_data;
    crypto_t *crypto = &arr->data[priv->index];
    crypto->timestamp = key->timestamp;
    crypto->close_price = db_crypto->close_price;
    crypto->volume = db_crypto->volume;
    crypto->liq_ask = db_crypto->liq_ask;
    crypto->liq_bid = db_crypto->liq_bid;
    crypto->whales = db_crypto->whales;
    priv->index++;
    return true;
}

bool db_crypto_sym_get(app_t *app, crypto_sym_t *out)
{
    const db_crypto_meta_t *meta = db_crypto_get_meta(app);
    if(meta == NULL) {
        return false;
    }
    uint16_t *offsets = (uint16_t *)meta->data;
    for(uint32_t i = 0; i < meta->sumbol_count; i++) {
        out->symbols[i] = meta->data + offsets[i];
    }
    out->count = meta->sumbol_count;
    return true;
}

bool db_crypto_get(app_t *app, crypto_arr_t *arr, const char *symbol, uint64_t start_ts, uint64_t end_ts,
                   uint32_t interval)
{
    db_key_crypto_t key_min, key_max;
    if(!db_crypto_to_key(app, &key_min, symbol, start_ts) || !db_crypto_to_key(app, &key_max, symbol, end_ts)) {
        return false;
    }
    crypto_iterate_priv_t priv = {
        .arr = arr,
        .index = 0,
        .interval = interval,
    };
    bool res = db_iterate(app, DB_TYPE, &key_min, &key_max, sizeof(key_max), db_crypto_iterate_cb, &priv);
    arr->count = priv.index;
    return res;
}

bool db_crypto_add(app_t *app, const char *symbol, const crypto_t *crypto)
{
    db_key_crypto_t key;
    if(!db_crypto_to_key(app, &key, symbol, crypto->timestamp)) {
        return false;
    }
    db_crypto_t db_crypto = {
        .close_price = crypto->close_price,
        .volume = crypto->volume,
        .liq_ask = crypto->liq_ask,
        .liq_bid = crypto->liq_bid,
        .whales = crypto->whales,
    };
    if(!db_put(app, DB_TYPE, &key, sizeof(key), &db_crypto, sizeof(db_crypto))) {
        return false;
    }
    return db_txn_commit(app);
}
