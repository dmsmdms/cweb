#include <core/http/http-client-mime.h>
#include <core/base/log.h>
#include <inttypes.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define MAX_MIMES_COUNT (16)
#define GEN_BUF_SIZE    (128 * 1024)

chttp_err_t chttp_post_gen(const char *url, const chttp_mime_gen_t *mimes, uint32_t mimes_count, chttp_resp_cb_t cb,
                           const buf_t *user_data)
{
    chttp_mime_t dst_mimes[MAX_MIMES_COUNT];
    if(mimes_count > MAX_MIMES_COUNT) {
        log_error("too many mimes: %u (max: %u)", mimes_count, MAX_MIMES_COUNT);
        return CHTTP_ERR_MEM_ALLOC;
    }

    str_buf_t buf;
    char mem_buf[GEN_BUF_SIZE];
    str_buf_init(&buf, mem_buf, sizeof(mem_buf));

    for(uint32_t i = 0; i < mimes_count; i++) {
        const chttp_mime_gen_t *mime = &mimes[i];
        chttp_mime_t *dst_mime = &dst_mimes[i];
        dst_mime->type = mime->type;
        dst_mime->name = mime->name;
        dst_mime->data = mime->cb(&buf, mime->val);
        buf_putc(&buf, '\0');
        if(buf.offset >= buf.size) {
            log_error("buffer overflow");
            return CHTTP_ERR_MEM_ALLOC;
        }
    }

    return chttp_post(url, dst_mimes, mimes_count, cb, user_data);
}

const char *chttp_mime_gen_str(UNUSED str_buf_t *buf, chttp_mime_val_t val)
{
    return val.str;
}

const char *chttp_mime_gen_uint32(str_buf_t *buf, chttp_mime_val_t val)
{
    char *p = buf->data + buf->offset;
    buf_printf(buf, "%" PRIu32, val.u32);
    return p;
}

const char *chttp_mime_gen_uint64(str_buf_t *buf, chttp_mime_val_t val)
{
    char *p = buf->data + buf->offset;
    buf_printf(buf, "%" PRIu64, val.u64);
    return p;
}

const char *chttp_mime_gen_jenum_arr(str_buf_t *buf, chttp_mime_val_t val)
{
    const chttp_mime_enum_arr_t *arr = val.enum_arr;
    char *p = buf->data + buf->offset;
    buf_putc(buf, '[');
    for(uint32_t i = 0; i < arr->count; i++) {
        uint32_t enum_val = arr->arr[i];
        buf_printf(buf, "\"%s\",", arr->enums[enum_val]);
    }
    if(arr->count > 0) {
        buf->offset--; // Remove last comma
    }
    buf_putc(buf, ']');
    return p;
}
