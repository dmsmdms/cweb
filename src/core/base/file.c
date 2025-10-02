#include <core/base/file.h>
#include <core/base/log.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define STREAM_BUF_SIZE (1024 * 1024)

file_err_t file_read_str(const char *path, str_t *content)
{
    int fd = open(path, O_RDONLY);
    if(fd < 0) {
        log_error("open file %s failed: %s", path, strerror(errno));
        return FILE_ERR_OPEN;
    }

    ssize_t n = read(fd, content->data, content->len);
    close(fd);
    if(n < 0) {
        log_error("read file %s failed: %s", path, strerror(errno));
        return FILE_ERR_READ;
    }
    if((size_t)n == content->len) {
        log_error("buffer[%zu] too small for file %s", content->len, path);
        return FILE_ERR_BUF_SMALL;
    }

    content->data[n] = '\0';
    content->len = n;
    return FILE_ERR_OK;
}

file_err_t file_read(const char *path, void *buf, uint32_t len)
{
    int fd = open(path, O_RDONLY);
    if(fd < 0) {
        log_error("open file %s failed: %s", path, strerror(errno));
        return FILE_ERR_OPEN;
    }
    ssize_t n = read(fd, buf, len);
    close(fd);
    if(n < 0) {
        log_error("read file %s failed: %s", path, strerror(errno));
        return FILE_ERR_READ;
    }
    if((uint32_t)n != len) {
        log_error("read file %s incomplete: %zu of %u", path, n, len);
        return FILE_ERR_READ;
    }
    return FILE_ERR_OK;
}

file_err_t file_write(const char *path, const void *data, uint32_t len)
{
    int fd = creat(path, 0644);
    if(fd < 0) {
        log_error("create file %s failed: %s", path, strerror(errno));
        return FILE_ERR_OPEN;
    }
    ssize_t n = write(fd, data, len);
    close(fd);
    if(n < 0) {
        log_error("write file %s failed: %s", path, strerror(errno));
        return FILE_ERR_WRITE;
    }
    if((uint32_t)n != len) {
        log_error("write file %s incomplete: %zu of %u", path, n, len);
        return FILE_ERR_WRITE;
    }
    return FILE_ERR_OK;
}

file_err_t file_stream(const char *path, file_stream_cb_t cb, void *priv_data)
{
    int fd = open(path, O_RDONLY);
    if(fd < 0) {
        log_error("open file %s failed: %s", path, strerror(errno));
        return FILE_ERR_OPEN;
    }

    uint32_t offset = 0;
    char buf[STREAM_BUF_SIZE];
    bool need_more = true;
    while(need_more) {
        int32_t n = read(fd, buf + offset, sizeof(buf) - 1 - offset);
        if(n < 0) {
            log_error("read file %s failed: %s", path, strerror(errno));
            close(fd);
            return FILE_ERR_READ;
        }
        if(n == 0) {
            break;
        }
        if((uint32_t)n < sizeof(buf) - 1 - offset) {
            need_more = false;
        }
        offset += n;
        buf[offset] = '\0';

        int32_t l = cb(buf, offset, priv_data);
        if(l < 0) {
            close(fd);
            return FILE_ERR_PARSE;
        }
        offset -= l;
        memmove(buf, buf + l, offset);
    }
    close(fd);
    return FILE_ERR_OK;
}
