#include <core/base/file.h>
#include <core/base/log.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

file_err_t file_read(const char *path, str_t *content)
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
