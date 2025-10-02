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

args_err_t args_parse(args_t *args, int argc, char *argv[])
{
    bzero(args, sizeof(args_t));
    args->log_lvl = LOG_LVL_WARN;

    char optstr[256] = "c:l:v:m:ds";
#ifdef CONFIG_DB
    strcat(optstr, "i:e:");
#endif
    while(true) {
        int opt = getopt(argc, argv, optstr);
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
        case 'd':
            args->run_daemon = true;
            break;
        case 's':
            args->stop_daemon = true;
            break;
        case 'v': {
            log_lvl_t lvl = log_lvl_parse(optarg);
            if(lvl == LOG_LVL_MAX) {
                log_error("invalid log level: -v %s", optarg);
                return ARGS_ERR_INVALID_PARAM;
            }
            args->log_lvl = lvl;
        } break;
        case 'm': {
            char *prm[2];
            if(str_split(optarg, ':', prm, ARRAY_SIZE(prm)) < 0) {
                log_error("invalid param: -m %s", optarg);
                return ARGS_ERR_INVALID_PARAM;
            }
            log_lvl_t lvl = log_lvl_parse(prm[1]);
            if(lvl == LOG_LVL_MAX) {
                log_error("invalid log level: -m %s", optarg);
                return ARGS_ERR_INVALID_PARAM;
            }
            log_mod_set_lvl(prm[0], lvl);
        } break;
#ifdef CONFIG_DB
        case 'i': {
            char *prm[3];
            if(str_split(optarg, ':', prm, ARRAY_SIZE(prm)) != ARRAY_SIZE(prm)) {
                log_error("invalid param: -i %s", optarg);
                return ARGS_ERR_INVALID_PARAM;
            }
            args->db_table = prm[0];
            args->db_prm = prm[1];
            args->db_import_file = prm[2];
        } break;
        case 'e': {
            char *prm[3];
            if(str_split(optarg, ':', prm, ARRAY_SIZE(prm)) != ARRAY_SIZE(prm)) {
                log_error("invalid param: -e %s", optarg);
                return ARGS_ERR_INVALID_PARAM;
            }
            args->db_table = prm[0];
            args->db_prm = prm[1];
            args->db_export_file = prm[2];
        } break;
#endif
        default:
            return ARGS_ERR_INVALID_PARAM;
        }
    }
    if(args->run_daemon && args->log_file == NULL) {
        log_error("daemon requires log file: -l <file>");
        return ARGS_ERR_INVALID_PARAM;
    }
#ifdef CONFIG_DB
    if(args->db_import_file && args->db_export_file) {
        log_error("cannot use both -i and -e options simultaneously");
        return ARGS_ERR_INVALID_PARAM;
    }
#endif
    return ARGS_ERR_OK;
}
