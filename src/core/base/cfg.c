#include <core/base/cfg.h>
#include <core/base/file.h>
#include <core/base/log.h>
#include <core/json/json-parser.h>
#include <malloc.h>
#include <string.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define CFG_BUF_SIZE (64 * 1024)

static char *data = NULL;

cfg_err_t cfg_parse(cfg_t *cfg, const char *cfg_file)
{
    bzero(cfg, sizeof(cfg_t));
    cfg->pid_file = "tmp/cweb.pid";
#ifdef CONFIG_DB
    cfg->db_path = "tmp/db";
    cfg->db_size_mb = 64;
#endif
#ifdef CONFIG_HTTP_SERVER
    cfg->http_sock = "tmp/http.sock";
    cfg->html_path = "tmp/html";
#endif
#if defined(CONFIG_IPC_SERVER) || defined(CONFIG_IPC_CLIENT)
    cfg->ipc_sock = "tmp/ipc.sock";
#endif
#ifdef CONFIG_TELEBOT
    cfg->bot_upd_sec = 30;
#endif
#ifdef CONFIG_LANG
    cfg->lang_path = "tmp/lang.json";
#endif
#ifdef CONFIG_DB_CRYPTO_TABLE
    cfg->crypto_list_path = "config/crypto-list.json";
#endif
#ifdef CONFIG_PARSER_CVBANKAS
    cfg->parser_cvb_upd_sec = 5;
#endif

    if(cfg_file == NULL) {
        return CFG_ERR_OK;
    }
    char buf[CFG_BUF_SIZE];
    str_t file = {
        .data = buf,
        .len = sizeof(buf),
    };
    if(file_read_str(cfg_file, &file) != FILE_ERR_OK) {
        return CFG_ERR_PARSE;
    }

    json_item_t items[] = {
        { "pid_file", json_parse_pstr, &cfg->pid_file },
#ifdef CONFIG_DB
        { "db_path", json_parse_pstr, &cfg->db_path },
        { "db_size_mb", json_parse_int32, &cfg->db_size_mb },
        { "db_count", json_parse_int32, &cfg->db_count },
#endif
#ifdef CONFIG_HTTP_SERVER
        { "http_sock", json_parse_pstr, &cfg->http_sock },
        { "html_path", json_parse_pstr, &cfg->html_path },
#endif
#if defined(CONFIG_IPC_SERVER) || defined(CONFIG_IPC_CLIENT)
        { "ipc_sock", json_parse_pstr, &cfg->ipc_sock },
#endif
#ifdef CONFIG_TELEBOT
        { "bot_token", json_parse_pstr, &cfg->bot_token },
        { "bot_upd_sec", json_parse_int32, &cfg->bot_upd_sec },
#endif
#ifdef CONFIG_LANG
        { "lang_path", json_parse_pstr, &cfg->lang_path },
#endif
#ifdef CONFIG_DB_CRYPTO_TABLE
        { "crypto_list_path", json_parse_pstr, &cfg->crypto_list_path },
#endif
#ifdef CONFIG_PARSER_CVBANKAS
        { "parser_cvb_upd_sec", json_parse_int32, &cfg->parser_cvb_upd_sec },
#endif
    };
    if(json_parse(file.data, file.len, items, ARRAY_SIZE(items)) != JSON_PARSE_ERR_OK) {
        return CFG_ERR_PARSE;
    }
    if(json_parse_pstr_dup(&data, &file, items, ARRAY_SIZE(items)) != JSON_PARSE_ERR_OK) {
        return CFG_ERR_NO_MEM;
    }
    return CFG_ERR_OK;
}

void cfg_free(void)
{
    free(data);
    data = NULL;
}
