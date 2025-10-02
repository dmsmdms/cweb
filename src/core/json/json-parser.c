#include <global.h>

#define MAX_TOKS 4096

static const char *jsmn_strerr[] = {
    [~JSMN_ERROR_NOMEM] = "no memory",
    [~JSMN_ERROR_INVAL] = "invalid character",
    [~JSMN_ERROR_PART] = "not full JSON data",
};

static bool json_streq(const char *json, const jsmntok_t *tok, const char *str)
{
    size_t tok_len = tok->end - tok->start;
    if(strlen(str) != tok_len) {
        return false;
    }
    return strncmp(json + tok->start, str, tok_len) == 0;
}

bool json_parse_wrap(app_t *app, const char *json, uint32_t json_size, const json_item_t *items, uint32_t num_items)
{
    jsmn_parser parser;
    jsmntok_t *toks = mem_alloc(app, __func__, MAX_TOKS * sizeof(jsmntok_t));
    if(toks == NULL) {
        return false;
    }
    jsmn_init(&parser);
    int n = jsmn_parse(&parser, json, json_size, toks, MAX_TOKS - 1);
    if(n < 0) {
        log_error("parse failed - %s: %.*s", jsmn_strerr[~n], json_size, json);
        return false;
    }
    toks[n].type = JSMN_UNDEFINED;
    return json_parse_obj(app, toks, json, items, num_items);
}

static const json_item_t *json_find_item(const jsmntok_t *key, const char *json, const json_item_t *items,
                                         uint32_t num_items)
{
    for(uint32_t i = 0; i < num_items; i++) {
        const json_item_t *item = &items[i];
        if(item->name == NULL) {
            return item; // default item parser
        }
        if(json_streq(json, key, item->name)) {
            return item;
        }
    }
    return NULL;
}

bool json_parse_obj(app_t *app, const jsmntok_t *cur, const char *json, const json_item_t *items, uint32_t num_items)
{
    if(cur->type != JSMN_OBJECT) {
        log_error("not an object");
        return false;
    }
    const jsmntok_t *key = cur + 1;
    if(key->type == JSMN_UNDEFINED) {
        return true;
    }
    while(true) {
        if(key->type != JSMN_STRING) {
            log_error("not a string key");
            return false;
        }
        const jsmntok_t *val = key + 1;
        const json_item_t *item = json_find_item(key, json, items, num_items);
        if(item) {
            if(item->cb && !item->cb(app, val, json, item->priv_data)) {
                return false;
            }
        } else {
            uint32_t tok_len = key->end - key->start;
            log_error("unknown key: %.*s", tok_len, json + key->start);
            return false;
        }

        while(key->start < val->end) {
            key++;
            if(key->type == JSMN_UNDEFINED) {
                return true;
            }
        }
        if(key->start > cur->end) {
            return true;
        }
    }
    return true;
}

bool json_parse_arr(app_t *app, const jsmntok_t *cur, const char *json, json_parse_cb_t cb, void *priv_data)
{
    if(cur->type != JSMN_ARRAY) {
        log_error("not an array");
        return false;
    }
    const jsmntok_t *obj = cur + 1;
    if(obj->type == JSMN_UNDEFINED) {
        return true;
    }
    while(true) {
        if(!cb(app, obj, json, priv_data)) {
            return false;
        }
        const jsmntok_t *prev = obj;
        while(obj->start < prev->end) {
            obj++;
            if(obj->type == JSMN_UNDEFINED) {
                return true;
            }
        }
        if(obj->start >= cur->end) {
            return true;
        }
    }
    return true;
}
bool json_parse_float(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    if(cur->type != JSMN_PRIMITIVE) {
        log_error("not a primitive");
        return false;
    }
    float *out = priv_data;
    *out = atof(json + cur->start);
    return true;
}

bool json_parse_int32(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    if(cur->type != JSMN_PRIMITIVE) {
        log_error("not a primitive");
        return false;
    }
    int32_t *out = priv_data;
    *out = atoi(json + cur->start);
    return true;
}

bool json_parse_int64(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    if(cur->type != JSMN_PRIMITIVE) {
        log_error("not a primitive");
        return false;
    }
    int64_t *out = priv_data;
    *out = atoll(json + cur->start);
    return true;
}

bool json_parse_bool(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    if(cur->type != JSMN_PRIMITIVE) {
        log_error("not a primitive");
        return false;
    }
    bool *out = priv_data;
    *out = json_streq(json, cur, "true");
    return true;
}

bool json_parse_str(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    if(cur->type != JSMN_STRING) {
        log_error("not a string");
        return false;
    }
    char *str = priv_data;
    size_t len = cur->end - cur->start;
    memcpy(str, json + cur->start, len);
    str[len] = '\0';
    return true;
}

bool json_parse_nstr(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    if(cur->type != JSMN_STRING) {
        log_error("not a string");
        return false;
    }
    str_t *out = priv_data;
    size_t len = cur->end - cur->start;
    if(len + 1 >= out->len) {
        log_error("string too long %zu + 1 >= %zu", len, out->len);
        return false;
    }
    memcpy(out->data, json + cur->start, len);
    out->data[len] = '\0';
    out->len = len;
    return true;
}

bool json_parse_pstr(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    if(cur->type != JSMN_STRING) {
        log_error("not a string");
        return false;
    }
    size_t len = cur->end - cur->start;
    char *str = mem_alloc(app, __func__, len + 1);
    if(str == NULL) {
        return false;
    }
    memcpy(str, json + cur->start, len);
    str[len] = '\0';
    char **out = priv_data;
    *out = str;
    return true;
}

bool json_parse_enum(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    char buf[64];
    if(!json_parse_str(app, cur, json, buf)) {
        return false;
    }
    const json_enum_t *data = priv_data;
    for(uint32_t i = 0; i < data->enums_count; i++) {
        if(strcmp(buf, data->enums[i]) == 0) {
            *data->pval = i;
            return true;
        }
    }
    log_error("unknown enum value: %s", buf);
    return false;
}
