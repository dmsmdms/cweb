#pragma once

#include <common.h>

#define PATH_MAX_LEN 256

/**
 * @brief Contains parsed configuration options
 */
typedef struct {
#ifdef CONFIG_DB
    char db_path[PATH_MAX_LEN]; ///< Path to database file (default: "tmp/db")
    uint32_t db_size_mb;        ///< Database size in megabytes (default: 64MB)
#endif
#ifdef CONFIG_HTTP_SERVER
    char http_sock[PATH_MAX_LEN]; ///< Path to HTTP server socket (default: "tmp/http.sock")
    char html_path[PATH_MAX_LEN]; ///< Path to HTML files (default: NULL)
#endif
#ifdef CONFIG_TELEBOT
    char bot_token[64];   ///< Telegram bot token (default: NULL)
    uint32_t bot_upd_sec; ///< Wait unil user interacts with bot (default: 30sec)
#endif
#ifdef CONFIG_LANG
    char lang_path[256]; ///< Path to language files (default: NULL)
#endif
#ifdef CONFIG_DB_CRYPTO_TABLE
    char crypto_list_path[256]; ///< Path to cryptocurrency list (default: "config/crypto-list.json")
#endif
#ifdef CONFIG_PARSER_CVBANKAS
    uint32_t parser_cvb_upd_sec; ///< CVBankas update interval in seconds (default: 5sec)
#endif
} config_t;

/**
 * @brief Parse configuration file
 * @param app - [in] Pointer to the application structure
 * @return true on success, false otherwise
 */
bool cfg_parse(app_t *app);
