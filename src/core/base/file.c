#include <global.h>

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
