#include <global.h>

static bool cfg_parse_cb(app_t *app, const char *data, uint32_t size, void *priv_data)
{
    UNUSED(priv_data);
    config_t *cfg = &app->cfg;
    json_item_t items[] = {
#ifdef CONFIG_DB
        { "db_path", json_parse_str, cfg->db_path },
        { "db_size_mb", json_parse_int32, &cfg->db_size_mb },
#endif
#ifdef CONFIG_HTTP_SERVER
        { "http_sock", json_parse_str, cfg->http_sock },
#endif
#ifdef CONFIG_LANG
        { "lang_path", json_parse_str, cfg->lang_path },
#endif
#ifdef CONFIG_DB_CRYPTO_TABLE
        { "crypto_list_path", json_parse_str, cfg->crypto_list_path },
#endif
#ifdef CONFIG_PARSER_CVBANKAS
        { "parser_cvb_upd_sec", json_parse_int32, &cfg->parser_cvb_upd_sec },
#endif
#ifdef CONFIG_APP_JOBLY_API
        { "html_jobly_path", json_parse_str, cfg->html_jobly_path },
#endif
#ifdef CONFIG_APP_JOBLY_BOT
        { "bot_jobly_token", json_parse_str, cfg->bot_jobly_token },
        { "bot_jobly_upd_sec", json_parse_int32, &cfg->bot_jobly_upd_sec },
#endif
    };
    return json_parse_wrap(app, data, size, items, ARRAY_SIZE(items));
}

bool cfg_parse(app_t *app)
{
    config_t *cfg = &app->cfg;
    const args_t *args = &app->args;
#ifdef CONFIG_DB
    strcpy(cfg->db_path, "tmp/db");
    cfg->db_size_mb = 64;
#endif
#ifdef CONFIG_HTTP_SERVER
    strcpy(cfg->http_sock, "tmp/http.sock");
#endif
#ifdef CONFIG_DB_CRYPTO_TABLE
    strcpy(cfg->crypto_list_path, "config/crypto-list.json");
#endif
#ifdef CONFIG_PARSER_CVBANKAS
    cfg->parser_cvb_upd_sec = 5;
#endif
#ifdef CONFIG_APP_JOBLY_BOT
    cfg->bot_jobly_upd_sec = 30;
#endif
    if(args->cfg_file) {
        uint32_t mem_offset = mem_get_offset(app);
        bool res = file_read(app, args->cfg_file, cfg_parse_cb, NULL);
        if(res == false) {
            log_error("read config %s failed", args->cfg_file);
        }
        mem_put_offset(app, mem_offset);
        return res;
    }
    return true;
}
