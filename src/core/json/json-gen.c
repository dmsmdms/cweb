#include <global.h>

#define JSON_MIN_RES_SIZE 256

bool json_gen_wrap(app_t *app, str_t *out, const json_gen_item_t *items, uint32_t num_items)
{
    char *buf = out->data;
    if(!json_gen_obj(app, out, items, num_items)) {
        return false;
    }
    out->len = out->data - buf;
    out->data = buf;
    return true;
}

bool json_gen_obj(app_t *app, str_t *out, const json_gen_item_t *items, uint32_t num_items)
{
    str_append_cstr(out, "{");
    for(uint32_t i = 0; i < num_items; i++) {
        const json_gen_item_t *item = &items[i];
        if(i > 0) {
            str_append_cstr(out, ",");
        }
        str_append_cstr(out, "\"");
        str_append_cstr(out, item->name);
        str_append_cstr(out, "\":");
        if(!item->cb(app, out, item->priv_data)) {
            return false;
        }
        if(out->len < JSON_MIN_RES_SIZE) {
            log_error("JSON gen overflow");
            return false;
        }
    }
    str_append_cstr(out, "}");
    return true;
}

bool json_gen_float(app_t *app, str_t *out, const void *priv_data)
{
    UNUSED(app);
    float val = *(float *)priv_data;
    size_t n = snprintf(out->data, out->len, "%g", val);
    out->data += n;
    out->len -= n;
    return true;
}

bool json_gen_uint64(app_t *app, str_t *out, const void *priv_data)
{
    UNUSED(app);
    uint64_t val = *(uint64_t *)priv_data;
    size_t n = snprintf(out->data, out->len, "%" PRIu64, val);
    out->data += n;
    out->len -= n;
    return true;
}

bool json_gen_uint8(app_t *app, str_t *out, const void *priv_data)
{
    UNUSED(app);
    uint8_t val = *(uint8_t *)priv_data;
    size_t n = snprintf(out->data, out->len, "%" PRIu8, val);
    out->data += n;
    out->len -= n;
    return true;
}

bool json_gen_str(app_t *app, str_t *out, const void *priv_data)
{
    UNUSED(app);
    str_append_cstr(out, "\"");
    str_append_cstr(out, priv_data);
    str_append_cstr(out, "\"");
    return true;
}

bool json_gen_str_arr(app_t *app, str_t *out, const void *priv_data)
{
    UNUSED(app);
    const json_str_arr_t *str_arr = priv_data;
    str_append_cstr(out, "[");
    for(uint32_t i = 0; i < str_arr->size; i++) {
        if(i > 0) {
            str_append_cstr(out, ",");
        }
        str_append_cstr(out, "\"");
        str_append_cstr(out, str_arr->arr[i]);
        str_append_cstr(out, "\"");
    }
    str_append_cstr(out, "]");
    return true;
}
