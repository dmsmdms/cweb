#include <core/json/json-parser.h>
#include <core/base/log.h>
#include <core/base/str.h>
#include <core/base/buf.h>
#include <string.h>
#include <stdlib.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define MAX_TOKS   4096
#define MAX_STRDUP 128

static const char *jsmn_strerr[] = {
    [~JSMN_ERROR_NOMEM] = "no memory",
    [~JSMN_ERROR_INVAL] = "invalid character",
    [~JSMN_ERROR_PART] = "not full JSON data",
};
STATIC_ASSERT(ARRAY_SIZE(jsmn_strerr) == 3);

static bool json_streq(const char *json, const jsmntok_t *tok, const char *str)
{
    size_t tok_len = tok->end - tok->start;
    if(strlen(str) != tok_len) {
        return false;
    }
    return strncmp(json + tok->start, str, tok_len) == 0;
}

json_parse_err_t json_parse(const char *json, uint32_t json_size, const json_item_t *items, uint32_t num_items)
{
    jsmn_parser parser;
    jsmntok_t toks[MAX_TOKS];
    jsmn_init(&parser);

    int n = jsmn_parse(&parser, json, json_size, toks, MAX_TOKS - 1);
    if(n < 0) {
        log_error("parse failed - %s: %.*s", jsmn_strerr[~n], json_size, json);
        return JSON_PARSE_ERR_DECODE;
    }

    toks[n].type = JSMN_UNDEFINED;
    return json_parse_obj(toks, json, items, num_items);
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

json_parse_err_t json_parse_obj(const jsmntok_t *cur, const char *json, const json_item_t *items, uint32_t num_items)
{
    if(cur->type != JSMN_OBJECT) {
        log_error("not an object");
        return JSON_PARSE_ERR_INVALID;
    }
    const jsmntok_t *key = cur + 1;
    if(key->type == JSMN_UNDEFINED) {
        return JSON_PARSE_ERR_INVALID;
    }
    while(key->start < cur->end) {
        if(key->type != JSMN_STRING) {
            log_error("not a string key");
            return JSON_PARSE_ERR_INVALID;
        }
        const jsmntok_t *val = key + 1;
        const json_item_t *item = json_find_item(key, json, items, num_items);
        if(item) {
            if(item->cb) {
                json_parse_err_t res = item->cb(val, json, item->priv_data);
                if(res != JSON_PARSE_ERR_OK) {
                    return res;
                }
            }
        } else {
            uint32_t tok_len = key->end - key->start;
            log_error("unknown key: %.*s", tok_len, json + key->start);
            return JSON_PARSE_ERR_NO_KEY;
        }

        while(key->start <= val->end) {
            key++;
            if(key->type == JSMN_UNDEFINED) {
                return JSON_PARSE_ERR_OK;
            }
        }
    }
    return JSON_PARSE_ERR_OK;
}

json_parse_err_t json_parse_arr(const jsmntok_t *cur, const char *json, json_parse_cb_t cb, void *priv_data)
{
    if(cur->type != JSMN_ARRAY) {
        log_error("not an array");
        return JSON_PARSE_ERR_INVALID;
    }
    if(cur->size == 0) {
        return JSON_PARSE_ERR_OK;
    }
    const jsmntok_t *obj = cur + 1;
    if(obj->type == JSMN_UNDEFINED) {
        return JSON_PARSE_ERR_INVALID;
    }
    while(obj->start < cur->end) {
        json_parse_err_t res = cb(obj, json, priv_data);
        if(res != JSON_PARSE_ERR_OK) {
            return res;
        }
        const jsmntok_t *prev = obj;
        while(obj->start <= prev->end) {
            obj++;
            if(obj->type == JSMN_UNDEFINED) {
                return JSON_PARSE_ERR_OK;
            }
        }
    }
    return JSON_PARSE_ERR_OK;
}

json_parse_err_t json_parse_float(const jsmntok_t *cur, const char *json, void *priv_data)
{
    if(cur->type != JSMN_PRIMITIVE) {
        log_error("not a primitive");
        return JSON_PARSE_ERR_INVALID;
    }

    char *end = NULL;
    float *pval = priv_data;
    *pval = strtof(json + cur->start, &end);
    if(end != &json[cur->end]) {
        log_error("convert error");
        return JSON_PARSE_ERR_CONVERT;
    }
    return JSON_PARSE_ERR_OK;
}

json_parse_err_t json_parse_int32(const jsmntok_t *cur, const char *json, void *priv_data)
{
    if(cur->type != JSMN_PRIMITIVE) {
        log_error("not a primitive");
        return JSON_PARSE_ERR_INVALID;
    }

    char *end = NULL;
    int32_t *pval = priv_data;
    *pval = strtol(json + cur->start, &end, 10);
    if(end != &json[cur->end]) {
        log_error("convert error");
        return JSON_PARSE_ERR_CONVERT;
    }
    return JSON_PARSE_ERR_OK;
}

json_parse_err_t json_parse_int64(const jsmntok_t *cur, const char *json, void *priv_data)
{
    if(cur->type != JSMN_PRIMITIVE) {
        log_error("not a primitive");
        return JSON_PARSE_ERR_INVALID;
    }

    char *end = NULL;
    int64_t *pval = priv_data;
    *pval = strtoll(json + cur->start, &end, 10);
    if(end != &json[cur->end]) {
        log_error("convert error");
        return JSON_PARSE_ERR_CONVERT;
    }
    return JSON_PARSE_ERR_OK;
}

json_parse_err_t json_parse_bool(const jsmntok_t *cur, const char *json, void *priv_data)
{
    if(cur->type != JSMN_PRIMITIVE) {
        log_error("not a primitive");
        return JSON_PARSE_ERR_INVALID;
    }

    bool *pval = priv_data;
    if(json_streq(json, cur, "true")) {
        *pval = true;
    } else if(json_streq(json, cur, "false")) {
        *pval = false;
    } else {
        log_error("convert error");
        return JSON_PARSE_ERR_CONVERT;
    }
    return JSON_PARSE_ERR_OK;
}

json_parse_err_t json_parse_pstr(const jsmntok_t *cur, const char *json, void *priv_data)
{
    if(cur->type != JSMN_STRING) {
        log_error("not a string");
        return JSON_PARSE_ERR_INVALID;
    }

    const char **pval = priv_data;
    char *end = (char *)&json[cur->end];
    *pval = &json[cur->start];
    *end = '\0';
    return JSON_PARSE_ERR_OK;
}

json_parse_err_t json_parse_enum(const jsmntok_t *cur, const char *json, void *priv_data)
{
    char *str;
    json_parse_err_t res = json_parse_pstr(cur, json, &str);
    if(res != JSON_PARSE_ERR_OK) {
        return res;
    }

    const json_enum_t *data = priv_data;
    for(uint32_t i = 0; i < data->enums_count; i++) {
        if(strcmp(str, data->enums[i]) == 0) {
            *data->pval = i;
            return JSON_PARSE_ERR_OK;
        }
    }
    log_error("unknown enum value: %s", str);
    return JSON_PARSE_ERR_CONVERT;
}

json_parse_err_t json_parse_pstr_dup(char **pdata, const str_t *buf, const json_item_t *items, uint32_t num_items)
{
    const char *buf_end = buf->data + buf->len;
    buf_ext_t str_buf[MAX_STRDUP];
    uint32_t str_buf_count = 0;
    uint32_t tot_size = 0;
    for(uint32_t i = 0; i < num_items; i++) {
        const json_item_t *item = &items[i];
        if(item->cb == json_parse_pstr) {
            const char **pstr = item->priv_data;
            const char *str = *pstr;
            if((str >= buf->data) && (str < buf_end)) {
                if(str_buf_count >= MAX_STRDUP) {
                    log_error("too many strdup items");
                    return JSON_PARSE_ERR_NO_MEM;
                }
                buf_ext_t *sbuf = &str_buf[str_buf_count++];
                sbuf->data = (void *)pstr;
                sbuf->size = strlen(str) + 1;
                sbuf->offset = tot_size;
                tot_size += sbuf->size;
            }
        }
    }
    if(str_buf_count == 0) {
        *pdata = NULL;
        return JSON_PARSE_ERR_OK;
    }

    char *data = malloc(tot_size);
    if(data == NULL) {
        log_error("malloc(%u) failed", tot_size);
        return JSON_PARSE_ERR_NO_MEM;
    }
    for(uint32_t i = 0; i < str_buf_count; i++) {
        buf_ext_t *sbuf = &str_buf[i];
        const char **pstr = sbuf->data;
        const char *str = *pstr;
        char *dst = data + sbuf->offset;
        memcpy(dst, str, sbuf->size);
        *pstr = dst;
    }
    *pdata = data;
    return JSON_PARSE_ERR_OK;
}
