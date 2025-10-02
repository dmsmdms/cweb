#include <global.h>

#define INTERVAL_MAX_SEC 3600
#define LIMIT_MAX        10000

static bool parse_metrics(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    api_crypto_req_metrics_t *metrics = priv_data;
    str_t symbol = {
        .data = metrics->symbol,
        .len = sizeof(metrics->symbol),
    };
    json_item_t items[] = {
        { "symbol", json_parse_nstr, &symbol },          { "start", json_parse_int64, &metrics->start_date },
        { "end", json_parse_int64, &metrics->end_date }, { "interval", json_parse_int32, &metrics->interval_sec },
        { "limit", json_parse_int32, &metrics->limit },
    };
    if(!json_parse_obj(app, cur, json, items, ARRAY_SIZE(items))) {
        return false;
    }
    if(symbol.data[0] == '\0') {
        log_error("symbol missing");
        return false;
    }
    if(metrics->start_date <= 0 || metrics->end_date <= 0 || metrics->start_date >= metrics->end_date) {
        log_error("start_date=%" PRId64 " end_date=%" PRId64 " invalid", metrics->start_date, metrics->end_date);
        return false;
    }
    if(metrics->interval_sec <= 0 || metrics->interval_sec > INTERVAL_MAX_SEC) {
        log_error("interval=%" PRIu32 "invalid", metrics->interval_sec);
        return false;
    }
    if(metrics->limit <= 0 || metrics->limit > LIMIT_MAX) {
        log_error("limit=%" PRIu32 " invalid", metrics->limit);
        return false;
    }
    return true;
}

bool api_crypto_parse_data(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    api_crypto_req_t *req = priv_data;
    static const json_parse_cb_t cb_arr[] = {
        [API_CRYPTO_ACT_GET_METRICS] = parse_metrics,
    };
    if(req->act >= API_CRYPTO_ACT_MAX) {
        log_error("act=%u invalid", req->act);
        return false;
    }
    json_parse_cb_t cb = cb_arr[req->act];
    if(cb == NULL) {
        log_error("act=%u not implemented", req->act);
        return false;
    }
    return cb(app, cur, json, &req->data);
}
