#include <core/base/buf.h>
#include <string.h>

void buf_init_ext(buf_ext_t *buf, void *data, uint32_t size)
{
    buf->data = data;
    buf->size = size;
    buf->offset = 0;
}

void *buf_alloc(buf_ext_t *buf, uint32_t size)
{
    size = ROUND_UP(size, sizeof(void *));
    if(buf->offset + size > buf->size) {
        return NULL;
    }
    void *ptr = (char *)buf->data + buf->offset;
    buf->offset += size;
    return ptr;
}

void *buf_calloc(buf_ext_t *buf, uint32_t size)
{
    void *ptr = buf_alloc(buf, size);
    if(ptr) {
        bzero(ptr, size);
    }
    return ptr;
}

void buf_circ_init(buf_circ_t *buf, void *data, uint32_t size, uint32_t item_size)
{
    buf->data = data;
    buf->off = 0;
    buf->cnt = 0;
    buf->size = size;
    buf->item_size = item_size;
}

void buf_circ_add(buf_circ_t *buf, const void *item)
{
    uint32_t off = (buf->off + buf->cnt) % buf->size;
    char *dst = &buf->data[off * buf->item_size];
    memcpy(dst, item, buf->item_size);
    if(buf->cnt == buf->size) {
        buf->off = (buf->off + 1) % buf->size;
    } else {
        buf->cnt++;
    }
}

void buf_circ_del_cur(buf_circ_t *buf)
{
    buf->off = (buf->off + 1) % buf->size;
    if(buf->cnt > 0) {
        buf->cnt--;
    }
}

const void *buf_circ_get(const buf_circ_t *buf, uint32_t index)
{
    uint32_t off = (buf->off + index) % buf->size;
    return &buf->data[off * buf->item_size];
}

float buf_circ_get_float(const buf_circ_t *buf, uint32_t index)
{
    return *(float *)buf_circ_get(buf, index);
}
