#include <global.h>

#define COLOR_RED    "\x1b[31m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_GREEN  "\x1b[32m"
#define COLOR_CYAN   "\x1b[36m"
#define COLOR_RESET  "\x1b[0m"

static const char *log_level_str[] = {
    [LOG_LEVEL_ERROR] = "ERROR",
    [LOG_LEVEL_WARN] = "WARN",
    [LOG_LEVEL_INFO] = "INFO",
    [LOG_LEVEL_DEBUG] = "DEBUG",
};
static const char *log_level_color[] = {
    [LOG_LEVEL_ERROR] = COLOR_RED,
    [LOG_LEVEL_WARN] = COLOR_YELLOW,
    [LOG_LEVEL_INFO] = COLOR_GREEN,
    [LOG_LEVEL_DEBUG] = COLOR_CYAN,
};

bool log_init(log_t *log, const char *file, log_level_t level)
{
    app_t *app = container_of(log, app_t, log);
    log->level = level;
    log->fd = STDOUT_FILENO;
    log->colors_en = true;
    fclose(stdin);
    //fclose(stderr);

    if(file) {
        log->fd = creat(file, 0644);
        if(log->fd < 0) {
            log_error("open log file %s failed - %s", file, strerror(errno));
            return false;
        }
        fclose(stdout);
        log->colors_en = false;
    }

    return true;
}

void log_destroy(log_t *log)
{
    if(log->fd >= 0) {
        close(log->fd);
        log->fd = -1;
    }
}

void log_write(const log_t *log, log_level_t lvl, const char *file, int line, const char *func, const char *fmt, ...)
{
    if(lvl > log->level) {
        return;
    }

    struct timespec ts;
    struct tm tm_info;
    char buf[1024];
    char *p = buf;
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &tm_info);

    if(log->colors_en) {
        p += sprintf(p, "%s", log_level_color[lvl]);
    }
    p += strftime(p, sizeof(buf), "[%Y-%m-%d %H:%M:%S", &tm_info);
    p += sprintf(p, ".%03ld][%s][%s:%d %s] ", ts.tv_nsec / 1000000, log_level_str[lvl], file, line, func);

    va_list args;
    va_start(args, fmt);
    p += vsprintf(p, fmt, args);
    va_end(args);

    if(log->colors_en) {
        p += sprintf(p, "%s", COLOR_RESET);
    }
    *p++ = '\n';
    write(log->fd, buf, p - buf);
}
