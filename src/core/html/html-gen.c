#include <global.h>

#define HTML_BUF_SIZE (2 * 1024 * 1024)

typedef struct {
    str_t *content;
    const html_gen_func_t *funcs;
    uint32_t funcs_count;
} html_gen_t;

static bool html_gen_cb(app_t *app, const char *data, uint32_t size, void *priv_data)
{
    const html_gen_t *gen = priv_data;
    char *buf = mem_alloc(app, __func__, HTML_BUF_SIZE);
    if(buf == NULL) {
        return false;
    }

    uint32_t buf_len = 0;
    for(uint32_t i = 0; i < size; i++) {
        if(data[i] == '<' && data[i + 1] == '?' && (i + 2) < size) {
            const char *start = &data[i + 2];
            char *end = strchr(start, '?');
            if(end == NULL) {
                return false;
            }
            i = (end - data) + 1;
        } else {
            buf[buf_len] = data[i];
            buf_len++;
        }
    }

    str_t *content = gen->content;
    content->data = buf;
    content->len = buf_len;
    return true;
}

bool html_gen(app_t *app, http_srv_resp_t *resp, const char *dir, const char *path, const html_gen_func_t *funcs,
              uint32_t funcs_count)
{
    html_gen_t gen = {
        .content = &resp->content,
        .funcs = funcs,
        .funcs_count = funcs_count,
    };
    char buf[256];
    snprintf(buf, sizeof(buf) - 1, "%s/%s", dir, path);
    resp->content_type = HTTP_CONTENT_TYPE_HTML;
    return file_read(app, buf, html_gen_cb, &gen);
}
