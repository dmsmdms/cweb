#include <parser/parser-binance-priv.h>
#include <core/base/log.h>
#include <db/db-crypto.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

static bin_stream_t bin_str_stream(const char *str)
{
    if(!strcmp(str, "depth10")) {
        return BIN_STREAM_DEPTH;
    } else if(!strcmp(str, "kline_1m")) {
        return BIN_STREAM_KLINE;
    }
    return BIN_STREAM_MAX;
}

static json_parse_err_t parse_kline_main(const jsmntok_t *cur, const char *json, void *priv_data)
{
    bin_kline_t *kline = priv_data;
    char *close_str = NULL;
    char *volume_str = NULL;
    json_item_t items[] = {
        { "c", json_parse_pstr, &close_str },
        { "v", json_parse_pstr, &volume_str },
        { NULL, NULL, NULL },
    };
    json_parse_err_t res = json_parse_obj(cur, json, items, ARRAY_SIZE(items));
    if(res != JSON_PARSE_ERR_OK) {
        return res;
    }
    if(close_str == NULL || volume_str == NULL) {
        log_error("missing kline data");
        return JSON_PARSE_ERR_CONVERT;
    }

    char *end = close_str;
    kline->close = strtof(close_str, &end);
    if(end != &close_str[strlen(close_str)]) {
        log_error("convert close price error - %s", close_str);
        return JSON_PARSE_ERR_CONVERT;
    }
    end = volume_str;
    kline->volume = strtof(volume_str, &end);
    if(end != &volume_str[strlen(volume_str)]) {
        log_error("convert volume error - %s", volume_str);
        return JSON_PARSE_ERR_CONVERT;
    }
    return JSON_PARSE_ERR_OK;
}

static json_parse_err_t parse_kline(const jsmntok_t *cur, const char *json, void *priv_data)
{
    bin_kline_t *kline = priv_data;
    json_item_t items[] = {
        { "k", parse_kline_main, kline },
        { NULL, NULL, NULL },
    };
    return json_parse_obj(cur, json, items, ARRAY_SIZE(items));
}

static json_parse_err_t parse_liq_entry(const jsmntok_t *cur, const char *json, void *priv_data)
{
    bin_liq_entry_t *entry = priv_data;
    char *str_val;
    json_parse_err_t res = json_parse_pstr(cur, json, &str_val);
    if(res != JSON_PARSE_ERR_OK) {
        return res;
    }
    char *end = str_val;
    entry->data[entry->count] = strtof(str_val, &end);
    if(end != &str_val[strlen(str_val)]) {
        log_error("convert liq entry error - %s", str_val);
        return JSON_PARSE_ERR_CONVERT;
    }
    entry->count++;
    return JSON_PARSE_ERR_OK;
}

static json_parse_err_t parse_liq_arr(const jsmntok_t *cur, const char *json, void *priv_data)
{
    bin_liq_t *liq = priv_data;
    if(cur->size != BIN_DEPTH_PRM_MAX) {
        log_error("invalid liq entry size - %d", cur->size);
        return JSON_PARSE_ERR_NO_MEM;
    }
    return json_parse_arr(cur, json, parse_liq_entry, &liq->data[liq->count++]);
}

static json_parse_err_t parse_liq(const jsmntok_t *cur, const char *json, void *priv_data)
{
    if(cur->size != BIN_LIQ_COUNT) {
        log_error("invalid liq size - %d", cur->size);
        return JSON_PARSE_ERR_NO_MEM;
    }
    return json_parse_arr(cur, json, parse_liq_arr, priv_data);
}

static json_parse_err_t parse_depth(const jsmntok_t *cur, const char *json, void *priv_data)
{
    bin_depth_t *depth = priv_data;
    json_item_t items[] = {
        { "bids", parse_liq, &depth->bids },
        { "asks", parse_liq, &depth->asks },
        { NULL, NULL, NULL },
    };
    return json_parse_obj(cur, json, items, ARRAY_SIZE(items));
}

json_parse_err_t parser_bin_stream(const jsmntok_t *cur, const char *json, void *priv_data)
{
    bin_update_t *update = priv_data;
    char *sym;
    json_parse_err_t res = json_parse_pstr(cur, json, &sym);
    if(res != JSON_PARSE_ERR_OK) {
        return res;
    }
    char *argv[3];
    if(str_split(sym, '@', argv, ARRAY_SIZE(argv)) < 0) {
        log_error("invalid stream - %s", sym);
        return JSON_PARSE_ERR_CONVERT;
    }
    update->type = bin_str_stream(argv[1]);
    if(update->type == BIN_STREAM_MAX) {
        log_error("unknown stream type - %s", argv[1]);
        return JSON_PARSE_ERR_CONVERT;
    }
    update->symbol = sym;
    return JSON_PARSE_ERR_OK;
}

json_parse_err_t parser_bin_data(const jsmntok_t *cur, const char *json, void *priv_data)
{
    bin_update_t *update = priv_data;
    static const json_parse_cb_t cb[] = {
        [BIN_STREAM_KLINE] = parse_kline,
        [BIN_STREAM_DEPTH] = parse_depth,
    };
    return cb[update->type](cur, json, &update->data);
}

void parser_bin_calc_kline(const bin_update_t *update, const bin_depth_val_t *val, uint64_t *pts)
{
    const bin_kline_t *kline = &update->data.kline;
    if(val->liq_cnt == 0) {
        log_warn("skip kline for %s, no depth yet", update->symbol);
        return;
    }
    crypto_t crypto = {
        .ts = time(NULL),
        .close = kline->close,
        .volume = kline->volume,
        .liq_ask = val->liq_ask,
        .liq_bid = val->liq_bid,
        .whales = val->whales,
    };
    log_debug("symbol %s: time=%" PRIu64 " close=%g, volume=%g, liq_ask=%g, liq_bid=%g, whales=%u", update->symbol,
              crypto.ts, crypto.close, crypto.volume, crypto.liq_ask, crypto.liq_bid, crypto.whales);
    if(db_crypto_add(val->sym_id, &crypto) != DB_ERR_OK) {
        return;
    }
    *pts = crypto.ts;
}

void parser_bin_calc_depth(const bin_update_t *update, bin_depth_val_t *val)
{
    const bin_depth_t *depth = &update->data.depth;

    // Calculate liq_ask && liq_bid //
    float liq_ask[BIN_LIQ_COUNT];
    float liq_bid[BIN_LIQ_COUNT];
    val->liq_ask = 0;
    val->liq_bid = 0;
    for(uint32_t i = 0; i < BIN_LIQ_COUNT; i++) {
        const bin_liq_entry_t *asks = &depth->asks.data[i];
        const bin_liq_entry_t *bids = &depth->bids.data[i];
        liq_ask[i] = asks->data[BIN_DEPTH_PRM_PRICE] * asks->data[BIN_DEPTH_PRM_QTY];
        liq_bid[i] = bids->data[BIN_DEPTH_PRM_PRICE] * bids->data[BIN_DEPTH_PRM_QTY];
        val->liq_ask += liq_ask[i];
        val->liq_bid += liq_bid[i];
    }

    /* Calculate liq_avg */
    float liq_tot = val->liq_ask + val->liq_bid;
    if(val->liq_cnt > 0) {
        if(val->liq_cnt < BIN_LIQ_HIST) {
            val->liq_cnt += 2;
        }
        val->liq_avg = (val->liq_avg * (val->liq_cnt - 2) + liq_tot) / val->liq_cnt;
    } else {
        val->liq_avg = liq_tot;
        val->liq_cnt += 2;
    }

    /* Calculate whales */
    float whale_thresh = BIN_LIQ_TRESH * val->liq_avg;
    val->whales = 0;
    for(uint32_t i = 0; i < BIN_LIQ_COUNT; i++) {
        val->whales += liq_ask[i] >= whale_thresh;
        val->whales += liq_bid[i] >= whale_thresh;
    }
}
