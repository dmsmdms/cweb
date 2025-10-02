#include <global.h>

#define STREAM_STR_SIZE 64

static binance_stream_t binance_str_stream(const char *str)
{
    if(!strcmp(str, "depth10")) {
        return BINANCE_STREAM_DEPTH;
    } else if(!strcmp(str, "kline_1m")) {
        return BINANCE_STREAM_KLINE;
    }
    return BINANCE_STREAM_MAX;
}

bool parser_binance_stream(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    binance_update_t *update = priv_data;
    char *symbol = mem_alloc(app, __func__, STREAM_STR_SIZE);
    if(symbol == NULL) {
        return false;
    }
    if(!json_parse_str(app, cur, json, symbol)) {
        return false;
    }
    char *argv[3];
    if(str_split(symbol, '@', argv, ARRAY_SIZE(argv)) < 0) {
        log_error("invalid stream - %s", symbol);
        return false;
    }
    update->type = binance_str_stream(argv[1]);
    if(update->type == BINANCE_STREAM_MAX) {
        log_error("unknown stream type - %s", argv[1]);
        return false;
    }
    update->symbol = symbol;
    return true;
}

static bool parse_kline_main(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    binance_kline_t *kline = priv_data;
    char close_price_str[64];
    char volume_str[64];
    close_price_str[0] = '\0';
    volume_str[0] = '\0';
    json_item_t items[] = {
        { "t", NULL, NULL },
        { "s", NULL, NULL },
        { "i", NULL, NULL },
        { "f", NULL, NULL },
        { "L", NULL, NULL },
        { "o", NULL, NULL },
        { "h", NULL, NULL },
        { "l", NULL, NULL },
        { "n", NULL, NULL },
        { "x", NULL, NULL },
        { "q", NULL, NULL },
        { "V", NULL, NULL },
        { "Q", NULL, NULL },
        { "B", NULL, NULL },
        { "T", NULL, NULL },
        { "c", json_parse_str, close_price_str },
        { "v", json_parse_str, volume_str },
    };
    if(!json_parse_obj(app, cur, json, items, ARRAY_SIZE(items))) {
        return false;
    }
    if(close_price_str[0] == '\0' || volume_str[0] == '\0') {
        log_error("missing kline data");
        return false;
    }
    kline->close_price = atof(close_price_str);
    kline->volume = atof(volume_str);
    return true;
}

static bool parse_kline(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    binance_kline_t *kline = priv_data;
    json_item_t items[] = {
        { "e", NULL, NULL },
        { "E", NULL, NULL },
        { "s", NULL, NULL },
        { "k", parse_kline_main, kline },
    };
    return json_parse_obj(app, cur, json, items, ARRAY_SIZE(items));
}

static bool parse_liq_entry_arr(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    binance_liq_entry_t *entry = priv_data;
    if(entry->count >= ARRAY_SIZE(entry->data)) {
        log_error("too many liq entry items");
        return false;
    }
    char buf[64];
    if(!json_parse_str(app, cur, json, buf)) {
        return false;
    }
    entry->data[entry->count++] = atof(buf);
    return true;
}

static bool parse_liq_arr(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    binance_liq_t *liq = priv_data;
    if(liq->count >= ARRAY_SIZE(liq->data)) {
        log_error("too many liq entries");
        return false;
    }
    return json_parse_arr(app, cur, json, parse_liq_entry_arr, &liq->data[liq->count++]);
}

static bool parse_liq(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    return json_parse_arr(app, cur, json, parse_liq_arr, priv_data);
}

static bool parse_depth(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    binance_depth_t *depth = priv_data;
    json_item_t items[] = {
        { "lastUpdateId", NULL, NULL },
        { "bids", parse_liq, &depth->bids },
        { "asks", parse_liq, &depth->asks },
    };
    return json_parse_obj(app, cur, json, items, ARRAY_SIZE(items));
}

bool parser_binance_data(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    binance_update_t *update = priv_data;
    static const json_parse_cb_t cb[] = {
        [BINANCE_STREAM_KLINE] = parse_kline,
        [BINANCE_STREAM_DEPTH] = parse_depth,
    };
    return cb[update->type](app, cur, json, &update->data);
}
