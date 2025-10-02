#include <global.h>

static void gen_resp_get_symbols(app_t *app, http_srv_resp_t *resp, const api_crypto_req_t *req);
static void gen_resp_get_metrics(app_t *app, http_srv_resp_t *resp, const api_crypto_req_t *req);

static const char *act_map[] = {
    [API_CRYPTO_ACT_GET_SYMBOLS] = "get-symbols",
    [API_CRYPTO_ACT_GET_METRICS] = "get-metrics",
};
static const json_gen_item_t bad_req_items[] = {
    { "error", json_gen_str, "Bad request" },
};
static const json_gen_item_t mem_error_items[] = {
    { "error", json_gen_str, "Memory allocation error" },
};
static const json_gen_item_t db_error_items[] = {
    { "error", json_gen_str, "Database error" },
};
static const api_crypto_gen_resp_cb_t gen_resp_cb_arr[] = {
    [API_CRYPTO_ACT_GET_SYMBOLS] = gen_resp_get_symbols,
    [API_CRYPTO_ACT_GET_METRICS] = gen_resp_get_metrics,
};

static void gen_resp_get_symbols(app_t *app, http_srv_resp_t *resp, const api_crypto_req_t *req)
{
    UNUSED(req);
    crypto_sym_t sym;
    if(!db_crypto_sym_get(app, &sym)) {
        http_gen_resp_json(app, resp, HTTP_RESP_CODE_500_SERVER_ERROR, db_error_items, ARRAY_SIZE(db_error_items));
        return;
    }
    json_str_arr_t json_sym = {
        .arr = sym.symbols,
        .size = sym.count,
    };
    json_gen_item_t items[] = {
        { "sym", json_gen_str_arr, &json_sym },
    };
    http_gen_resp_json(app, resp, HTTP_RESP_CODE_200_OK, items, ARRAY_SIZE(items));
}

static bool json_gen_metrics(app_t *app, str_t *out, const void *priv_data)
{
    const crypto_arr_t *metrics = priv_data;
    json_gen_item_t items[] = {
        [API_CRYPTO_METRICS_TIMESTAMP] = { "ts", json_gen_uint64, NULL },
        [API_CRYPTO_METRICS_CLOSE_PRICE] = { "c", json_gen_float, NULL },
        [API_CRYPTO_METRICS_VOLUME] = { "v", json_gen_float, NULL },
        [API_CRYPTO_METRICS_LIQ_ASK] = { "la", json_gen_float, NULL },
        [API_CRYPTO_METRICS_LIQ_BID] = { "lb", json_gen_float, NULL },
        [API_CRYPTO_METRICS_WHALES] = { "w", json_gen_uint8, NULL },
    };
    str_append_cstr(out, "[");
    for(uint32_t i = 0; i < metrics->count; i++) {
        const crypto_t *metric = &metrics->data[i];
        items[API_CRYPTO_METRICS_TIMESTAMP].priv_data = &metric->timestamp;
        items[API_CRYPTO_METRICS_CLOSE_PRICE].priv_data = &metric->close_price;
        items[API_CRYPTO_METRICS_VOLUME].priv_data = &metric->volume;
        items[API_CRYPTO_METRICS_LIQ_ASK].priv_data = &metric->liq_ask;
        items[API_CRYPTO_METRICS_LIQ_BID].priv_data = &metric->liq_bid;
        items[API_CRYPTO_METRICS_WHALES].priv_data = &metric->whales;
        if(i > 0) {
            str_append_cstr(out, ",");
        }
        if(!json_gen_obj(app, out, items, ARRAY_SIZE(items))) {
            return false;
        }
    }
    str_append_cstr(out, "]");
    return true;
}

static void gen_resp_get_metrics(app_t *app, http_srv_resp_t *resp, const api_crypto_req_t *req)
{
    const api_crypto_req_metrics_t *req_metrics = &req->data.metrics;
    crypto_arr_t metrics;
    metrics.data = mem_alloc(app, __func__, req_metrics->limit * sizeof(crypto_t));
    if(metrics.data == NULL) {
        http_gen_resp_json(app, resp, HTTP_RESP_CODE_500_SERVER_ERROR, mem_error_items, ARRAY_SIZE(mem_error_items));
        return;
    }
    metrics.count = req_metrics->limit;
    if(!db_crypto_get(app, &metrics, req_metrics->symbol, req_metrics->start_date, req_metrics->end_date,
                      req_metrics->interval_sec)) {
        http_gen_resp_json(app, resp, HTTP_RESP_CODE_500_SERVER_ERROR, db_error_items, ARRAY_SIZE(db_error_items));
        return;
    }
    json_gen_item_t items[] = {
        { "metrics", json_gen_metrics, &metrics },
    };
    http_gen_resp_json(app, resp, HTTP_RESP_CODE_200_OK, items, ARRAY_SIZE(items));
}

void api_crypto_cb(const http_srv_req_t *http_req, http_srv_resp_t *resp)
{
    app_t *app = http_req->app;
    api_crypto_req_t req = {
        .act = API_CRYPTO_ACT_MAX,
    };
    json_enum_t act_enum = {
        .enums = act_map,
        .enums_count = ARRAY_SIZE(act_map),
        .pval = &req.act,
    };
    json_item_t items[] = {
        { "act", json_parse_enum, &act_enum },
        { "data", api_crypto_parse_data, &req },
    };
    uint32_t mem_offset = mem_get_offset(app);
    bool res = json_parse_wrap(app, http_req->data, http_req->size, items, ARRAY_SIZE(items));
    mem_put_offset(app, mem_offset);
    if(res == false) {
        http_gen_resp_json(app, resp, HTTP_RESP_CODE_400_BAD_REQUEST, bad_req_items, ARRAY_SIZE(bad_req_items));
        return;
    }
    api_crypto_gen_resp_cb_t cb = gen_resp_cb_arr[req.act];
    cb(app, resp, &req);
}
