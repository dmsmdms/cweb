#include <core/http/http-ext.h>
#include <core/json/json-gen.h>
#include <core/base/log.h>
#include <string.h>
#include <stdlib.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define MAX_JSON_BODY_SIZE (1024 * 1024)

http_parse_err_t http_parse_headers(const phr_header_t *headers, uint32_t num_headers, const http_header_item_t *items,
                                    uint32_t num_items)
{
    for(uint32_t i = 0; i < num_items; i++) {
        const http_header_item_t *item = &items[i];
        for(uint32_t j = 0; j < num_headers; j++) {
            const phr_header_t *header = &headers[j];
            char *h_name = (char *)header->name;
            h_name[header->name_len] = '\0';

            if(strcmp(item->name, h_name) == 0) {
                http_parse_err_t res = item->cb(header, item->priv_data);
                if(res != HTTP_PARSE_ERR_OK) {
                    return res;
                }
                goto next;
            }
        }

        log_error("header not found: %s", items[i].name);
        return HTTP_PARSE_ERR_NOT_FOUND;
    next:
        continue;
    }
    return HTTP_PARSE_ERR_OK;
}

http_parse_err_t http_parse_int32(const phr_header_t *header, void *priv_data)
{
    char *end = NULL;
    int32_t *pval = priv_data;
    *pval = strtol(header->value, &end, 10);
    if(end != &header->value[header->value_len]) {
        log_error("invalid int32 value: %s", header->value);
        return HTTP_PARSE_ERR_INVALID;
    }
    return HTTP_PARSE_ERR_OK;
}

http_parse_err_t http_parse_enum(const phr_header_t *header, void *priv_data)
{
    const http_enum_t *info = priv_data;
    char *h_value = (char *)header->value;
    h_value[header->value_len] = '\0';

    for(uint32_t i = 0; i < info->enums_count; i++) {
        if(strcmp(header->value, info->enums[i]) == 0) {
            *info->pval = i;
            return HTTP_PARSE_ERR_OK;
        }
    }
    log_error("unknown enum value: %s", header->value);
    return HTTP_PARSE_ERR_INVALID;
}

http_gen_err_t http_resp_json(shttp_conn_t *conn, shttp_resp_code_t code, shttp_content_type_t content_type,
                              shttp_connection_t connection, const json_gen_item_t *items, uint32_t num_items)
{
    char buf[MAX_JSON_BODY_SIZE];
    str_buf_t json = {
        .data = buf,
        .size = sizeof(buf),
    };
    if(json_gen_obj(&json, items, num_items) != JSON_GEN_ERR_OK) {
        return HTTP_GEN_ERR_JSON;
    }

    str_t body = {
        .data = json.data,
        .len = json.offset,
    };
    if(shttp_resp(conn, code, content_type, connection, &body) != SHTTP_ERR_OK) {
        return HTTP_GEN_ERR_SEND;
    }
    return HTTP_GEN_ERR_OK;
}
