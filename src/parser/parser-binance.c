#include <parser/parser-binance.h>
#include <parser/parser-binance-priv.h>
#include <core/ws/ws-client.h>
#include <core/base/log.h>
#include <db/db-crypto.h>
#include <search.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define BIN_ADDR     "stream.binance.com"
#define BIN_PORT     9443
#define BIN_WS_PATH  "/stream?streams="
#define BIN_SYNC_CNT 1000

typedef struct {
    struct hsearch_data depth_htab;
    bin_depth_val_t *depth_arr;
    cws_conn_t **conn;
    uint64_t start_ts;
    uint64_t last_upd_ts;
    uint32_t db_put_cnt;
    uint32_t depth_count;
    uint32_t conn_count;
} parser_bin_t;

static parser_bin_t *bin = NULL;

static bin_depth_val_t *depth_get_val(const char *sym)
{
    ENTRY *e_ptr, entry = {
        .key = (char *)sym,
    };
    if(hsearch_r(entry, FIND, &e_ptr, &bin->depth_htab) < 0) {
        log_error("symbol %s not found", sym);
        return NULL;
    }
    return e_ptr->data;
}

static void recv_cb(const cws_recv_t *recv)
{
    bin_update_t update = {};
    json_item_t items[] = {
        { "stream", parser_bin_stream, &update },
        { "data", parser_bin_data, &update },
    };
    if(json_parse(recv->body.data, recv->body.len, items, ARRAY_SIZE(items)) != JSON_PARSE_ERR_OK) {
        return;
    }
    bin_depth_val_t *val = depth_get_val(update.symbol);
    if(val == NULL) {
        return;
    }
    switch(update.type) {
    case BIN_STREAM_DEPTH:
        parser_bin_calc_depth(&update, val);
        break;
    case BIN_STREAM_KLINE:
        parser_bin_calc_kline(&update, val, &bin->last_upd_ts);
        bin->db_put_cnt++;
        if(bin->db_put_cnt >= BIN_SYNC_CNT) {
            db_sync();
            bin->db_put_cnt = 0;
        }
        break;
    default:
        break;
    }
}

parser_bin_err_t parser_bin_init(void)
{
    char sym_buf[2 * CRYPTO_SYM_ARR_BUF_SIZE];
    buf_ext_t buf = {
        .data = sym_buf,
        .size = sizeof(sym_buf),
    };
    crypto_sym_arr_t arr;
    if(db_crypto_sym_arr_get(&arr, &buf) != DB_ERR_OK) {
        return PARSER_BIN_ERR_DB;
    }

    // Calculate required memory size //
    uint32_t conn_count = (arr.count + BIN_MAX_STREAMS - 1) / BIN_MAX_STREAMS;
    uint32_t tot_size = sizeof(parser_bin_t);
    tot_size += arr.count * sizeof(bin_depth_val_t);
    tot_size += conn_count * sizeof(cws_conn_t *);
    bin = calloc(1, tot_size);
    if(bin == NULL) {
        log_error("calloc(%u) failed", tot_size);
        return PARSER_BIN_ERR_NO_MEM;
    }
    if(hcreate_r(2 * arr.count, &bin->depth_htab) < 0) {
        log_error("depth_htab[%u] create failed", 2 * arr.count);
        parser_bin_destroy();
        return PARSER_BIN_ERR_NO_MEM;
    }
    bin->depth_arr = (bin_depth_val_t *)&bin[1];
    bin->conn = (cws_conn_t **)&bin->depth_arr[arr.count];
    bin->depth_count = arr.count;
    bin->conn_count = conn_count;

    // Alloc connections path //
    char **path_arr = buf_alloc(&buf, 2 * conn_count * sizeof(char *));
    if(path_arr == NULL) {
        log_error("buf_alloc(%zu) failed", 2 * conn_count * sizeof(char *));
        parser_bin_destroy();
        return PARSER_BIN_ERR_NO_MEM;
    }
    char **p_arr = &path_arr[conn_count];
    for(uint32_t i = 0; i < conn_count; i++) {
        char *path = buf_alloc(&buf, BIN_MAX_WS_PATH_LEN);
        if(path == NULL) {
            log_error("buf_alloc(%d) failed", BIN_MAX_WS_PATH_LEN);
            parser_bin_destroy();
            return PARSER_BIN_ERR_NO_MEM;
        }
        path_arr[i] = path;
        p_arr[i] = path + sprintf(path, BIN_WS_PATH);
    }

    // Fill connections path and depth htab //
    for(uint32_t i = 0; i < arr.count; i++) {
        const crypto_sym_t *sym = &arr.data[i];
        bin_depth_val_t *val = &bin->depth_arr[i];
        if(strlen(sym->name) >= sizeof(val->sym_name)) {
            log_error("symbol %s is too long", sym->name);
            parser_bin_destroy();
            return PARSER_BIN_ERR_INVALID;
        }

        char **pp = &p_arr[i % conn_count];
        char *p = *pp;
        p += sprintf(p, "%s@kline_1m/", sym->name);
        p += sprintf(p, "%s@depth10@1000ms/", sym->name);
        strcpy(val->sym_name, sym->name);
        val->sym_id = sym->id;

        ENTRY *e_ptr, entry = {
            .key = val->sym_name,
            .data = val,
        };
        if(hsearch_r(entry, ENTER, &e_ptr, &bin->depth_htab) < 0) {
            log_error("htab insert failed for %s", sym->name);
            parser_bin_destroy();
            return PARSER_BIN_ERR_NO_MEM;
        }
        *pp = p;
    }

    // Establish connections //
    for(uint32_t i = 0; i < conn_count; i++) {
        p_arr[i][-1] = '\0';
        cws_conn_t *conn;
        if(cws_connect(&conn, BIN_ADDR, path_arr[i], BIN_PORT, true, recv_cb, NULL) != CWS_ERR_OK) {
            parser_bin_destroy();
            return PARSER_BIN_ERR_CONN;
        }
        bin->conn[i] = conn;
    }

    bin->start_ts = time(NULL);
    bin->last_upd_ts = bin->start_ts;
    return PARSER_BIN_ERR_OK;
}

void parser_bin_get_stat(parser_bin_stat_t *stat)
{
    stat->start_ts = bin->start_ts;
    stat->last_upd_ts = bin->last_upd_ts;
}

void parser_bin_destroy(void)
{
    if(bin == NULL) {
        return;
    }
    if(app_is_running) {
        for(uint32_t i = 0; i < bin->conn_count; i++) {
            cws_disconnect(bin->conn[i]);
        }
    }
    hdestroy_r(&bin->depth_htab);
    free(bin);
    bin = NULL;
}
