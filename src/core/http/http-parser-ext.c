#include <global.h>

bool http_parse_headers(const app_t *app, const phr_header_t *headers, uint32_t num_headers,
                        const http_header_item_t *items, uint32_t num_items)
{
    for(uint32_t i = 0; i < num_items; i++) {
        const http_header_item_t *item = &items[i];
        for(uint32_t j = 0; j < num_headers; j++) {
            const phr_header_t *header = &headers[j];
            char *h_name = (char *)header->name;
            h_name[header->name_len] = '\0';
            if(strcmp(item->name, h_name) == 0) {
                if(!item->cb(app, header, item->priv_data)) {
                    return false;
                }
                goto next;
            }
        }
        log_error("header not found: %s", items[i].name);
        return false;
    next:
        continue;
    }
    return true;
}

bool http_parse_int32(const app_t *app, const phr_header_t *header, void *priv_data)
{
    UNUSED(app);
    uint32_t *pval = priv_data;
    *pval = atoi(header->value);
    return true;
}

bool http_parse_enum(const app_t *app, const phr_header_t *header, void *priv_data)
{
    const http_enum_t *info = priv_data;
    char *h_value = (char *)header->value;
    h_value[header->value_len] = '\0';
    for(uint32_t i = 0; i < info->enums_count; i++) {
        if(strcmp(header->value, info->enums[i]) == 0) {
            *info->pval = i;
            return true;
        }
    }
    log_error("unknown enum value: %s", header->value);
    return false;
}
