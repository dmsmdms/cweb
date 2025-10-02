#include <global.h>
#define NO_DEBUG
#include <core/base/log-off.h>

#define BINANCE_LIQ_HIST  200
#define BINANCE_LIQ_TRESH 0.2f
#define BINANCE_WS_PATH   "/stream?streams="

static binance_depth_val_t *depth_get_val(app_t *app, const char *sym)
{
    parser_binance_t *binance = &app->parser_binance;
    ENTRY *e_ptr, entry = {
        .key = (char *)sym,
    };
    if(!hsearch_r(entry, FIND, &e_ptr, &binance->depth_htab)) {
        log_error("symbol %s not found", sym);
        return NULL;
    }
    return e_ptr->data;
}

static void calc_depth(const app_t *app, const binance_update_t *update, binance_depth_val_t *val)
{
    const binance_depth_t *depth = &update->data.depth;
    if(depth->bids.count != BINANCE_LIQ_COUNT || depth->asks.count != BINANCE_LIQ_COUNT) {
        log_error("invalid depth count: bids %u, asks %u", depth->bids.count, depth->asks.count);
        return;
    }

    /* Calculate liq_ask && liq_bid */
    float liq_ask[BINANCE_LIQ_COUNT];
    float liq_bid[BINANCE_LIQ_COUNT];
    val->liq_ask = 0;
    val->liq_bid = 0;
    for(uint32_t i = 0; i < BINANCE_LIQ_COUNT; i++) {
        const binance_liq_entry_t *asks = &depth->asks.data[i];
        const binance_liq_entry_t *bids = &depth->bids.data[i];
        liq_ask[i] = asks->data[BINANCE_DEPTH_PRM_PRICE] * asks->data[BINANCE_DEPTH_PRM_QTY];
        liq_bid[i] = bids->data[BINANCE_DEPTH_PRM_PRICE] * bids->data[BINANCE_DEPTH_PRM_QTY];
        val->liq_ask += liq_ask[i];
        val->liq_bid += liq_bid[i];
    }

    /* Calculate liq_avg */
    float liq_tot = val->liq_ask + val->liq_bid;
    if(val->liq_cnt > 0) {
        if(val->liq_cnt < BINANCE_LIQ_HIST) {
            val->liq_cnt += 2;
        }
        val->liq_avg = (val->liq_avg * (val->liq_cnt - 2) + liq_tot) / val->liq_cnt;
    } else {
        val->liq_avg = liq_tot;
        val->liq_cnt += 2;
    }

    /* Calculate whales */
    float whale_thresh = BINANCE_LIQ_TRESH * val->liq_avg;
    val->whales = 0;
    for(uint32_t i = 0; i < BINANCE_LIQ_COUNT; i++) {
        val->whales += liq_ask[i] >= whale_thresh;
        val->whales += liq_bid[i] >= whale_thresh;
    }
}

static void calc_kline(app_t *app, const binance_update_t *update, binance_depth_val_t *val)
{
    const binance_kline_t *kline = &update->data.kline;
    if(kline->close_price == 0) {
        log_error("invalid kline data: close_price=%g", kline->close_price);
        return;
    }
    if(val->liq_cnt == 0) {
        log_warn("skip kline for %s, no depth yet", update->symbol);
        return;
    }
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    crypto_t crypto = {
        .timestamp = ts.tv_sec,
        .close_price = kline->close_price,
        .volume = kline->volume,
        .liq_ask = val->liq_ask,
        .liq_bid = val->liq_bid,
        .whales = val->whales,
    };
    log_debug("symbol %s: time=%" PRIu64 " close_price=%g, volume=%g, liq_ask=%g, liq_bid=%g, whales=%u",
              update->symbol, crypto.timestamp, crypto.close_price, crypto.volume, crypto.liq_ask, crypto.liq_bid,
              crypto.whales);
    db_crypto_add(app, update->symbol, &crypto);
}

static void recv_cb(const ws_recv_t *recv)
{
    app_t *app = recv->app;
    binance_update_t update = {};
    json_item_t items[] = {
        { "stream", parser_binance_stream, &update },
        { "data", parser_binance_data, &update },
    };
    uint32_t mem_offset = mem_get_offset(app);
    if(!json_parse_wrap(app, recv->data, recv->size, items, ARRAY_SIZE(items))) {
        mem_put_offset(app, mem_offset);
        return;
    }
    binance_depth_val_t *val = depth_get_val(app, update.symbol);
    if(val == NULL) {
        mem_put_offset(app, mem_offset);
        return;
    }
    switch(update.type) {
    case BINANCE_STREAM_DEPTH:
        calc_depth(app, &update, val);
        break;
    case BINANCE_STREAM_KLINE:
        calc_kline(app, &update, val);
        break;
    default:
        break;
    }
    mem_put_offset(app, mem_offset);
}

bool parser_binance_init(app_t *app)
{
    parser_binance_t *binance = &app->parser_binance;
    crypto_sym_t syms;
    if(!db_crypto_sym_get(app, &syms)) {
        return false;
    }
    if(syms.count == 0) {
        log_error("no crypto symbols found");
        return false;
    }
    if(hcreate_r(2 * syms.count, &binance->depth_htab) < 0) {
        log_error("failed to create depth htab");
        return false;
    }
    binance->depth_arr = calloc(1, syms.count * sizeof(binance_depth_val_t));
    if(binance->depth_arr == NULL) {
        log_error("alloc depth_arr failed");
        parser_binance_destroy(app);
        return false;
    }
    binance->symbol_cnt = syms.count;

    uint32_t mem_offset = mem_get_offset(app);
    uint32_t conn_count = (syms.count + BINANCE_MAX_STREAMS - 1) / BINANCE_MAX_STREAMS;
    char *path_arr[conn_count];
    char *p_arr[conn_count];
    for(uint32_t i = 0; i < conn_count; i++) {
        char *path = mem_alloc(app, __func__, 64 * syms.count);
        if(path == NULL) {
            parser_binance_destroy(app);
            mem_put_offset(app, mem_offset);
            return false;
        }
        path_arr[i] = path;
        p_arr[i] = path + sprintf(path, BINANCE_WS_PATH);
    }

    for(uint32_t i = 0; i < syms.count; i++) {
        const char *sym = syms.symbols[i];
        binance_depth_val_t *val = &binance->depth_arr[i];
        if(strlen(sym) >= sizeof(val->symbol)) {
            log_error("symbol %s is too long", sym);
            parser_binance_destroy(app);
            mem_put_offset(app, mem_offset);
            return false;
        }

        char **pp = &p_arr[i % conn_count];
        char *p = *pp;
        p += sprintf(p, "%s@kline_1m/", sym);
        p += sprintf(p, "%s@depth10@1000ms/", sym);
        strcpy(val->symbol, sym);

        ENTRY *e_ptr, entry = {
            .key = val->symbol,
            .data = val,
        };
        if(!hsearch_r(entry, ENTER, &e_ptr, &binance->depth_htab)) {
            log_error("htab insert failed for %s", sym);
            parser_binance_destroy(app);
            mem_put_offset(app, mem_offset);
            return false;
        }
        *pp = p;
    }

    for(uint32_t i = 0; i < conn_count; i++) {
        p_arr[i][-1] = '\0';
        ws_conn_t *conn = ws_connect(app, "stream.binance.com", path_arr[i], 9443, true, recv_cb, NULL);
        if(conn == NULL) {
            parser_binance_destroy(app);
            mem_put_offset(app, mem_offset);
            return false;
        }
        binance->conns[i] = conn;
    }
    mem_put_offset(app, mem_offset);
    return true;
}

void parser_binance_destroy(app_t *app)
{
    parser_binance_t *binance = &app->parser_binance;
    if(app->is_running) {
        for(uint32_t i = 0; i < BINANCE_MAX_CONNS; i++) {
            ws_disconnect(binance->conns[i]);
            binance->conns[i] = NULL;
        }
    }
    hdestroy_r(&binance->depth_htab);
    free(binance->depth_arr);
    binance->depth_htab.table = NULL;
    binance->depth_arr = NULL;
}
