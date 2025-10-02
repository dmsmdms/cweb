#include <global.h>

#define MAX_JSON_GEN_SIZE (1024 * 1024)

bool http_gen_resp_json(app_t *app, http_srv_resp_t *resp, http_resp_code_t code, const json_gen_item_t *items,
                        uint32_t num_items)
{
    str_t *content = &resp->content;
    resp->code = code;
    resp->content_type = HTTP_CONTENT_TYPE_JSON;
    content->data = mem_alloc(app, __func__, MAX_JSON_GEN_SIZE);
    if(content->data == NULL) {
        return false;
    }
    content->len = MAX_JSON_GEN_SIZE;
    return json_gen_wrap(app, content, items, num_items);
}
