#include <global.h>

#define FILE_STREAM_BUF_SIZE (1024 * 1024)

bool file_read(app_t *app, const char *path, file_read_cb_t cb, void *priv_data)
{
    int fd = open(path, O_RDONLY);
    if(fd < 0) {
        return false;
    }
    struct stat st;
    if(fstat(fd, &st) < 0) {
        close(fd);
        return false;
    }
    char *data = mem_alloc(app, __func__, st.st_size + 1);
    if(data == NULL) {
        close(fd);
        return false;
    }
    ssize_t read_bytes = read(fd, data, st.st_size);
    close(fd);
    if(read_bytes != st.st_size) {
        return false;
    }
    data[st.st_size] = '\0';
    return cb(app, data, st.st_size, priv_data);
}

static void file_stream_end(app_t *app, int fd, uint32_t mem_offset)
{
    mem_put_offset(app, mem_offset);
    close(fd);
}

bool file_stream(app_t *app, const char *path, file_stream_cb_t cb, void *priv_data)
{
    int fd = creat(path, 0644);
    if(fd < 0) {
        return false;
    }
    uint32_t mem_offset = mem_get_offset(app);
    char *buf = mem_alloc(app, __func__, FILE_STREAM_BUF_SIZE);
    if(buf == NULL) {
        file_stream_end(app, fd, mem_offset);
        return false;
    }

    bool res = true;
    while(res) {
        str_t out = {
            .data = buf,
            .len = FILE_STREAM_BUF_SIZE,
        };
        res = cb(app, &out, priv_data);
        out.len = out.data - buf;

        ssize_t n = write(fd, buf, out.len);
        if(n < 0) {
            log_error("file %s write(%d, ..., %zu) failed - %s", path, fd, out.len, strerror(errno));
            file_stream_end(app, fd, mem_offset);
            return false;
        }
        if((size_t)n != out.len) {
            log_error("file %s write(%d, ..., %zu) wrote %zd bytes only", path, fd, out.len, n);
            file_stream_end(app, fd, mem_offset);
            return false;
        }
    }

    file_stream_end(app, fd, mem_offset);
    return true;
}
