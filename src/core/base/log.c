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

bool log_init(app_t *app)
{
    log_t *log = &app->log;
    args_t *args = &app->args;
    log->fd = STDOUT_FILENO;
    fclose(stdin);
#ifdef CONFIG_BUILD_RELEASE
    fclose(stderr);
#endif

    if(args->log_file) {
        log->fd = creat(args->log_file, 0644);
        if(log->fd < 0) {
            log_error("open log file %s failed - %s", args->log_file, strerror(errno));
            return false;
        }
        fclose(stdout);
    }

    return true;
}

void log_destroy(app_t *app)
{
    log_t *log = &app->log;
    if(log->fd >= 0) {
        close(log->fd);
        log->fd = -1;
    }
}

void log_write(const app_t *app, log_level_t lvl, const char *file, int line, const char *func, const char *fmt, ...)
{
    const log_t *log = &app->log;
    const args_t *args = &app->args;
    if(lvl > args->log_level) {
        return;
    }
    const char *fname = strrchr(file, '/');
    if(fname) {
        file = fname + 1;
    }

    struct timespec ts;
    struct tm tm_info;
    char buf[64 * 1024];
    char *p = buf;
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &tm_info);

    if(args->log_file == NULL) {
        p += sprintf(p, "%s", log_level_color[lvl]);
    }
    p += strftime(p, sizeof(buf), "[%Y-%m-%d %H:%M:%S", &tm_info);
    p += sprintf(p, ".%03ld][%s][%s:%d %s] ", ts.tv_nsec / 1000000, log_level_str[lvl], file, line, func);

    va_list va_args;
    va_start(va_args, fmt);
    p += vsprintf(p, fmt, va_args);
    va_end(va_args);

    if(args->log_file == NULL) {
        p += sprintf(p, "%s", COLOR_RESET);
    }
    *p++ = '\n';
    write(log->fd, buf, p - buf);
}
