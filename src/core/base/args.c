#include <global.h>

static log_level_t log_level_parse(const char *level)
{
    if(!strcmp(level, "debug")) {
        return LOG_LEVEL_DEBUG;
    } else if(!strcmp(level, "info")) {
        return LOG_LEVEL_INFO;
    } else if(!strcmp(level, "warn")) {
        return LOG_LEVEL_WARN;
    } else if(!strcmp(level, "error")) {
        return LOG_LEVEL_ERROR;
    }
    return LOG_LEVEL_WARN;
}

void args_parse(app_t *app, int argc, char *argv[])
{
    args_t *args = &app->args;
    args->log_level = LOG_LEVEL_WARN;
    while(true) {
        int opt = getopt(argc, argv, "c:v:l:");
        if(opt == -1) {
            break;
        }
        switch(opt) {
        case 'c':
            args->cfg_file = optarg;
            break;
        case 'l':
            args->log_file = optarg;
            break;
        case 'v':
            args->log_level = log_level_parse(optarg);
            break;
        default:
            break;
        }
    }
}
