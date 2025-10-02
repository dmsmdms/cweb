#include <core/base/buf.h>
#include <string.h>

void *buf_alloc(buf_ext_t *buf, size_t size)
{
    size = ROUND_UP(size, sizeof(void *));
    if(buf->offset + size > buf->size) {
        return NULL;
    }
    void *ptr = (char *)buf->data + buf->offset;
    buf->offset += size;
    return ptr;
}

void *buf_calloc(buf_ext_t *buf, size_t size)
{
    void *ptr = buf_alloc(buf, size);
    if(ptr) {
        bzero(ptr, size);
    }
    return ptr;
}
