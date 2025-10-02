#include <db/db-crypto.h>
#include <db/db-crypto-table.h>
#include <core/json/json-parser.h>
#include <core/csv/csv-parser.h>
#include <core/csv/csv-gen.h>
#include <core/base/file.h>
#include <core/base/log.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ev.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define DB_TXN_SIZE (128 * 1024)

typedef enum {
    CRYPTO_CSV_COL_TS,
    CRYPTO_CSV_COL_PRICE,
    CRYPTO_CSV_COL_VOLUME,
    CRYPTO_CSV_COL_LIQ_ASK,
    CRYPTO_CSV_COL_LIQ_BID,
    CRYPTO_CSV_COL_WHALES,
    CRYPTO_CSV_COL_MAX,
} crypto_csv_col_t;

typedef struct {
    crypto_sym_arr_t arr;
    buf_ext_t buf;
} crypto_sym_arr_gen_t;

typedef struct {
    db_cursor_op_t op;
    uint32_t sym_id;
    uint32_t line_count;
} crypto_csv_parse_t;

static const char *const csv_col_names[] = {
    [CRYPTO_CSV_COL_TS] = "timestamp",    [CRYPTO_CSV_COL_PRICE] = "price",     [CRYPTO_CSV_COL_VOLUME] = "volume",
    [CRYPTO_CSV_COL_LIQ_ASK] = "liq_ask", [CRYPTO_CSV_COL_LIQ_BID] = "liq_bid", [CRYPTO_CSV_COL_WHALES] = "whales",
};
STATIC_ASSERT(ARRAY_SIZE(csv_col_names) == CRYPTO_CSV_COL_MAX);

static json_parse_err_t json_parse_crypto_sym(const jsmntok_t *cur, const char *json, void *priv_data)
{
    crypto_sym_arr_gen_t *gen = priv_data;
    crypto_sym_t *sym = &gen->arr.data[gen->arr.count++];
    return json_parse_pstr(cur, json, &sym->name);
}

static json_parse_err_t json_parse_crypto_arr(const jsmntok_t *cur, const char *json, void *priv_data)
{
    crypto_sym_arr_gen_t *gen = priv_data;
    gen->arr.data = buf_alloc(&gen->buf, cur->size * sizeof(crypto_sym_t));
    if(gen->arr.data == NULL) {
        return JSON_PARSE_ERR_NO_MEM;
    }
    return json_parse_arr(cur, json, json_parse_crypto_sym, gen);
}

db_err_t db_crypto_init(const char *crypto_list_path)
{
    db_crypto_meta_t meta;
    db_err_t res = db_txn_begin(false);
    if(res != DB_ERR_OK) {
        return res;
    }
    res = db_crypto_get_meta(&meta);
    if(res != DB_ERR_OK) {
        db_txn_abort();
        return res;
    }
    if(meta.sym_count > 0) {
        db_txn_abort();
        return DB_ERR_OK;
    }

    // Read default crypto list //
    char buf[CRYPTO_SYM_ARR_BUF_SIZE];
    str_t file = {
        .data = buf,
        .len = sizeof(buf),
    };
    if(file_read_str(crypto_list_path, &file) != FILE_ERR_OK) {
        db_txn_abort();
        return DB_ERR_OPEN;
    }

    // Parse JSON //
    char json_buf[CRYPTO_SYM_ARR_BUF_SIZE];
    crypto_sym_arr_gen_t gen = {
        .buf.data = json_buf,
        .buf.size = sizeof(json_buf),
    };
    json_item_t items[] = {
        { "global", json_parse_crypto_arr, &gen },
    };
    if(json_parse(file.data, file.len, items, ARRAY_SIZE(items)) != JSON_PARSE_ERR_OK) {
        db_txn_abort();
        return DB_ERR_PARSE;
    }

    // Store symbols in DB //
    for(uint32_t i = 0; i < gen.arr.count; i++) {
        res = db_crypto_put_sym(gen.arr.data[i].name, i + 1);
        if(res != DB_ERR_OK) {
            db_txn_abort();
            return res;
        }
    }

    // Update meta //
    meta.sym_count = gen.arr.count;
    meta.sym_id_last = gen.arr.count;
    res = db_crypto_put_meta(&meta);
    if(res != DB_ERR_OK) {
        db_txn_abort();
        return res;
    }

    return db_txn_commit();
}

static csv_parse_err_t csv_parse_row(const csv_parse_ctx_t *pctx, const char **cols, const uint32_t cols_count,
                                     void *priv_data)
{
    crypto_csv_parse_t *ctx = priv_data;
    uint64_t ts = 0;
    db_crypto_t crypto = { 0 };
    csv_item_t items[] = {
        [CRYPTO_CSV_COL_TS] = { csv_parse_ts, &ts },
        [CRYPTO_CSV_COL_PRICE] = { csv_parse_float, &crypto.close },
        [CRYPTO_CSV_COL_VOLUME] = { csv_parse_float, &crypto.volume },
        [CRYPTO_CSV_COL_LIQ_ASK] = { csv_parse_float, &crypto.liq_ask },
        [CRYPTO_CSV_COL_LIQ_BID] = { csv_parse_float, &crypto.liq_bid },
        [CRYPTO_CSV_COL_WHALES] = { csv_parse_uint8, &crypto.whales },
    };
    csv_parse_err_t res = csv_parse(pctx, cols, cols_count, items, ARRAY_SIZE(items));
    if(res != CSV_PARSE_ERR_OK) {
        return res;
    }

    // Allow event loop to process events //
    if(ctx->line_count % DB_TXN_SIZE == 0) {
        if(db_txn_commit() != DB_ERR_OK) {
            return CSV_PARSE_ERR_INVALID;
        }
        ev_run(EV_DEFAULT, EVRUN_NOWAIT);
        if(app_is_running == false) {
            return CSV_PARSE_ERR_ABORT;
        }
    }
    if(db_crypto_put(ctx->sym_id, ts, &crypto) != DB_ERR_OK) {
        return CSV_PARSE_ERR_INVALID;
    }
    ctx->line_count++;
    return CSV_PARSE_ERR_OK;
}

db_err_t db_crypto_import_csv(const char *csv_path, const char *sym_name)
{
    crypto_csv_parse_t ctx = {
        .line_count = 0,
    };
    db_err_t res = db_crypto_get_sym(sym_name, &ctx.sym_id);
    if(res != DB_ERR_OK) {
        if(res == DB_ERR_NOT_FOUND) {
            log_error("Symbol '%s' not found in DB", sym_name);
        }
        db_txn_abort();
        return res;
    }
    if(csv_parse_file(csv_path, csv_parse_row, csv_col_names, ARRAY_SIZE(csv_col_names), &ctx) != CSV_PARSE_ERR_OK) {
        db_txn_abort();
        return DB_ERR_PARSE;
    }
    log_info("imported %u rows for '%s'", ctx.line_count, sym_name);
    return db_txn_commit();
}

static csv_gen_err_t csv_gen_row(const csv_gen_ctx_t *gctx, void *priv_data)
{
    crypto_csv_parse_t *ctx = priv_data;
    crypto_t crypto;
    db_err_t res = db_crypto_get_next1(ctx->sym_id, ctx->op, &crypto);
    if(res != DB_ERR_OK) {
        if(res == DB_ERR_NOT_FOUND) {
            return CSV_GEN_ERR_EOF;
        }
        return CSV_GEN_ERR_DATA;
    }
    ctx->op = DB_CURSOR_OP_NEXT;
    csv_gen_item_t items[] = {
        [CRYPTO_CSV_COL_TS] = CSV_GEN_TS(crypto.ts),
        [CRYPTO_CSV_COL_PRICE] = CSV_GEN_FLOAT(crypto.close),
        [CRYPTO_CSV_COL_VOLUME] = CSV_GEN_FLOAT(crypto.volume),
        [CRYPTO_CSV_COL_LIQ_ASK] = CSV_GEN_FLOAT(crypto.liq_ask),
        [CRYPTO_CSV_COL_LIQ_BID] = CSV_GEN_FLOAT(crypto.liq_bid),
        [CRYPTO_CSV_COL_WHALES] = CSV_GEN_UINT8(crypto.whales),
    };
    // Allow event loop to process events //
    if(ctx->line_count % DB_TXN_SIZE == 0) {
        ev_run(EV_DEFAULT, EVRUN_NOWAIT);
        if(app_is_running == false) {
            return CSV_GEN_ERR_EOF;
        }
    }
    csv_gen_err_t csv_res = csv_gen(gctx, items, ARRAY_SIZE(items));
    if(csv_res != CSV_GEN_ERR_OK) {
        return csv_res;
    }
    ctx->line_count++;
    return CSV_GEN_ERR_OK;
}

db_err_t db_crypto_export_csv(const char *csv_path, const char *sym_name)
{
    if(strcmp(sym_name, "all") == 0) {
        // Create directory if not exists //
        if(access(csv_path, F_OK) < 0) {
            if(mkdir(csv_path, 0755) < 0) {
                log_error("mkdir(%s) failed - %s", csv_path, strerror(errno));
                return DB_ERR_OPEN;
            }
        }

        // Get all symbols //
        char buf_mem[CRYPTO_SYM_ARR_BUF_SIZE];
        buf_ext_t buf;
        buf_init_ext(&buf, buf_mem, sizeof(buf_mem));
        crypto_sym_arr_t arr;
        db_err_t res = db_crypto_sym_arr_get(&arr, &buf);
        if(res != DB_ERR_OK) {
            return res;
        }

        // Export each symbol //
        for(uint32_t i = 0; i < arr.count; i++) {
            const crypto_sym_t *sym = &arr.data[i];
            crypto_csv_parse_t ctx = {
                .op = DB_CURSOR_OP_SET_RANGE,
                .sym_id = sym->id,
                .line_count = 0,
            };
            char path[FILE_PATH_LEN_MAX];
            snprintf(path, sizeof(path), "%s/%s.csv", csv_path, sym->name);

            csv_gen_err_t csv_err = csv_gen_file(path, csv_gen_row, csv_col_names, ARRAY_SIZE(csv_col_names), &ctx);
            db_txn_abort();
            if(csv_err != CSV_GEN_ERR_OK) {
                return DB_ERR_PARSE;
            }
            log_info("exported %u rows to '%s'", ctx.line_count, path);
        }
    } else {
        // Get symbol ID //
        crypto_csv_parse_t ctx = {
            .op = DB_CURSOR_OP_SET_RANGE,
            .line_count = 0,
        };
        db_err_t res = db_crypto_get_sym(sym_name, &ctx.sym_id);
        if(res != DB_ERR_OK) {
            if(res == DB_ERR_NOT_FOUND) {
                log_error("Symbol '%s' not found in DB", sym_name);
            }
            db_txn_abort();
            return res;
        }

        // Export specified symbol //
        csv_gen_err_t csv_err = csv_gen_file(csv_path, csv_gen_row, csv_col_names, ARRAY_SIZE(csv_col_names), &ctx);
        db_txn_abort();
        if(csv_err != CSV_GEN_ERR_OK) {
            return DB_ERR_PARSE;
        }
        log_info("exported %u rows for '%s'", ctx.line_count, sym_name);
    }
    return DB_ERR_OK;
}

db_err_t db_crypto_add(uint32_t sym_id, const crypto_t *crypto)
{
    db_crypto_t db_crypto = {
        .close = crypto->close,
        .volume = crypto->volume,
        .liq_ask = crypto->liq_ask,
        .liq_bid = crypto->liq_bid,
        .whales = crypto->whales,
        .pad = { 0 },
    };
    db_err_t res = db_crypto_put(sym_id, crypto->ts, &db_crypto);
    if(res != DB_ERR_OK) {
        db_txn_abort();
        return res;
    }
    return db_txn_commit();
}

db_err_t db_crypto_sym_arr_get(crypto_sym_arr_t *arr, buf_ext_t *buf)
{
    db_crypto_meta_t meta;
    db_err_t res = db_crypto_get_meta(&meta);
    if(res != DB_ERR_OK) {
        db_txn_abort();
        return res;
    }
    arr->data = buf_alloc(buf, meta.sym_count * sizeof(crypto_sym_t));
    if(arr->data == NULL) {
        log_error("buf_alloc(%zu) failed", meta.sym_count * sizeof(crypto_sym_t));
        db_txn_abort();
        return DB_ERR_NO_MEM;
    }
    arr->count = meta.sym_count;

    // Retrieve symbols //
    for(uint32_t i = 0; i < meta.sym_count; i++) {
        crypto_sym_t *sym = &arr->data[i];
        uint32_t name_len;
        res = db_crypto_get_sym_next(&sym->name, &name_len, &sym->id);
        if(res != DB_ERR_OK) {
            return res;
        }
        char *name_str = buf_alloc(buf, name_len + 1);
        if(name_str == NULL) {
            log_error("buf_alloc(%u) failed", name_len + 1);
            return DB_ERR_NO_MEM;
        }
        memcpy(name_str, sym->name, name_len);
        name_str[name_len] = '\0';
        sym->name = name_str;
    }

    db_txn_abort();
    return DB_ERR_OK;
}
