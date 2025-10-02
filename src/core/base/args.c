#include <core/base/args.h>
#include <core/base/str.h>
#include <getopt.h>
#include <string.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

static log_lvl_t log_lvl_parse(const char *lvl)
{
    if(!strcmp(lvl, "debug")) {
        return LOG_LVL_DEBUG;
    } else if(!strcmp(lvl, "info")) {
        return LOG_LVL_INFO;
    } else if(!strcmp(lvl, "warn")) {
        return LOG_LVL_WARN;
    } else if(!strcmp(lvl, "error")) {
        return LOG_LVL_ERROR;
    }
    return LOG_LVL_MAX;
}

void args_parse(args_t *args, int argc, char *argv[])
{
    args->log_lvl = LOG_LVL_WARN;
    while(true) {
        int opt = getopt(argc, argv, "c:v:l:m:");
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
        case 'v': {
            log_lvl_t lvl = log_lvl_parse(optarg);
            if(lvl == LOG_LVL_MAX) {
                log_error("invalid log level: -v %s", optarg);
                break;
            }
            args->log_lvl = lvl;
        } break;
        case 'm': {
            char *prm[2];
            if(str_split(optarg, ':', prm, ARRAY_SIZE(prm)) < 0) {
                log_error("invalid param: -m %s", optarg);
                break;
            }
            log_lvl_t lvl = log_lvl_parse(prm[1]);
            if(lvl == LOG_LVL_MAX) {
                log_error("invalid log level: -m %s", optarg);
                break;
            }
            log_mod_set_lvl(prm[0], lvl);
        } break;
        }
    }
}
