#include <global.h>

#define HTML_BUF_SIZE (2 * 1024 * 1024)

typedef struct {
    str_t *content;
    const html_gen_func_t *funcs;
    uint32_t funcs_count;
} html_gen_t;

static int html_get_func(app_t *app, char *buf, const char *func, char *args, const html_gen_func_t *funcs,
                         uint32_t funcs_count)
{
    char *argv[8];
    int argc = str_split(args, ',', argv, ARRAY_SIZE(argv));
    if(argc < 0) {
        log_error("too many args");
        return -1;
    }
    for(uint32_t i = 0; i < funcs_count; i++) {
        const html_gen_func_t *f = &funcs[i];
        if(strcmp(func, f->name) == 0) {
            return f->cb(app, buf, argc, argv);
        }
    }
    log_error("func %s not found", func);
    return -1;
}

static bool html_gen_cb(app_t *app, const char *data, uint32_t size, void *priv_data)
{
    const html_gen_t *gen = priv_data;
    char *buf = mem_alloc(app, __func__, HTML_BUF_SIZE);
    if(buf == NULL) {
        return false;
    }
    str_t *content = gen->content;
    content->data = buf;

    const char *src = data, *end = data + size;
    while(src < end && (buf - content->data) < HTML_BUF_SIZE - 1) {
        if(src[1] == '?' && src[0] == '<') {
            const char *func = src + 3; // skip <? and space
            if(func >= end) {
                log_error("no func after <?");
                return false;
            }
            char *args = strchr(func, '(');
            if(args == NULL) {
                log_error("no ( after func");
                return false;
            }
            *args++ = '\0';
            char *end_args = strchr(args, ')');
            if(end_args == NULL) {
                log_error("no ) after args");
                return false;
            }
            *end_args++ = '\0';
            int n = html_get_func(app, buf, func, args, gen->funcs, gen->funcs_count);
            if(n < 0) {
                return false;
            }
            buf += n;
            src = end_args + 1;
            if(src >= end || src[0] != '?' || src[1] != '>') {
                log_error("no ?> after func");
                return false;
            }
        } else {
            *buf++ = *src++;
        }
    }
    *buf = '\0';
    content->len = buf - content->data;
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
