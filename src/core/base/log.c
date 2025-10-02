#include <core/base/log.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define COLOR_RED    "\x1b[31m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_GREEN  "\x1b[32m"
#define COLOR_CYAN   "\x1b[36m"
#define COLOR_RESET  "\x1b[0m"

static const char *const lvl_str[] = {
    [LOG_LVL_ERROR] = "ERROR",
    [LOG_LVL_WARN] = "WARN",
    [LOG_LVL_INFO] = "INFO",
    [LOG_LVL_DEBUG] = "DEBUG",
};
static const char *const lvl_color[] = {
    [LOG_LVL_ERROR] = COLOR_RED,
    [LOG_LVL_WARN] = COLOR_YELLOW,
    [LOG_LVL_INFO] = COLOR_GREEN,
    [LOG_LVL_DEBUG] = COLOR_CYAN,
};
static SLIST_HEAD(log_mod_list, log_mod) log_mods = SLIST_HEAD_INITIALIZER(log_mods);
static log_lvl_t log_lvl = LOG_LVL_WARN;
static bool log_color = true;

void log_mod_init(log_mod_t *mod)
{
    const char *fname = strrchr(mod->fname, '/');
    if(fname) {
        mod->fname = fname + 1;
    }
    SLIST_INSERT_HEAD(&log_mods, mod, entry);
}

log_err_t log_mod_set_lvl(const char *fname, log_lvl_t lvl)
{
    log_mod_t *mod;
    SLIST_FOREACH(mod, &log_mods, entry)
    {
        if(!strcmp(mod->fname, fname)) {
            mod->lvl = lvl;
            return LOG_ERR_OK;
        }
    }
    return LOG_ERR_NOT_FOUND;
}

log_err_t log_init(log_lvl_t lvl, const char *log_file)
{
    fclose(stdin);
    fclose(stderr);

    if(log_file) {
        FILE *out = freopen(log_file, "a", stdout);
        if(out == NULL) {
            log_error("open log file %s failed - %s", log_file, strerror(errno));
            return LOG_ERR_FILE_OPEN;
        }
        log_color = false;
    }

    int fd = fileno(stdout);
    if(dup2(fd, STDERR_FILENO) < 0) {
        log_error("dup2 failed - %s", strerror(errno));
        return LOG_ERR_SYS;
    }

    log_lvl = lvl;
    return LOG_ERR_OK;
}

void log_destroy(void)
{
    fclose(stdout);
}

void log_write(const log_mod_t *mod, log_lvl_t lvl, uint32_t line, const char *func, const char *fmt, ...)
{
    log_lvl_t cur_lvl = mod->lvl != LOG_LVL_DEFAULT ? mod->lvl : log_lvl;
    if(lvl > cur_lvl) {
        return;
    }

    char tbuf[64];
    struct timespec ts;
    struct tm tm;
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &tm);
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", &tm);

    if(log_color) {
        fputs(lvl_color[lvl], stdout);
    }
    printf("[%s.%03ld][%s][%s:%d %s] ", tbuf, ts.tv_nsec / 1000000, lvl_str[lvl], mod->fname, line, func);

    va_list va_args;
    va_start(va_args, fmt);
    vprintf(fmt, va_args);
    va_end(va_args);

    if(log_color) {
        fputs(COLOR_RESET, stdout);
    }
    putchar('\n');
    fflush(stdout);
}
