#include <core/json/json-gen.h>
#include <core/base/log.h>
#include <inttypes.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

json_gen_err_t json_gen_obj(str_buf_t *out, const json_gen_item_t *items, uint32_t num_items)
{
    buf_putc(out, '{');
    for(uint32_t i = 0; i < num_items; i++) {
        const json_gen_item_t *item = &items[i];
        buf_printf(out, "\"%s\":", item->name);

        json_gen_err_t res = item->cb(out, item->priv_data);
        if(res != JSON_GEN_ERR_OK) {
            return res;
        }
        if(out->offset >= out->size) {
            log_error("JSON gen overflow");
            return JSON_GEN_ERR_OVERFLOW;
        }
        buf_putc(out, ',');
    }
    out->data[out->offset - 1] = '}'; // replace last comma with }
    return JSON_GEN_ERR_OK;
}

json_gen_err_t json_gen_arr(str_buf_t *out, const json_gen_arr_t *info)
{
    buf_putc(out, '[');
    const char *pval = info->arr;
    for(uint32_t i = 0; i < info->count; i++) {
        json_gen_err_t res = info->cb(out, pval);
        if(res != JSON_GEN_ERR_OK) {
            return res;
        }
        if(out->offset >= out->size) {
            log_error("JSON gen overflow");
            return JSON_GEN_ERR_OVERFLOW;
        }
        pval += info->item_size;
        buf_putc(out, ',');
    }
    out->data[out->offset - 1] = ']'; // replace last comma with ]
    return JSON_GEN_ERR_OK;
}

json_gen_err_t json_gen_float(str_buf_t *out, const void *priv_data)
{
    float val = *(float *)priv_data;
    buf_printf(out, "%g", val);
    return JSON_GEN_ERR_OK;
}

json_gen_err_t json_gen_uint64(str_buf_t *out, const void *priv_data)
{
    uint64_t val = *(uint64_t *)priv_data;
    buf_printf(out, "%" PRIu64, val);
    return JSON_GEN_ERR_OK;
}

json_gen_err_t json_gen_uint8(str_buf_t *out, const void *priv_data)
{
    uint8_t val = *(uint8_t *)priv_data;
    buf_printf(out, "%" PRIu8, val);
    return JSON_GEN_ERR_OK;
}

json_gen_err_t json_gen_str(str_buf_t *out, const void *priv_data)
{
    const char *str = priv_data;
    buf_printf(out, "\"%s\"", str);
    return JSON_GEN_ERR_OK;
}
