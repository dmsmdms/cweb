#pragma once

#include <common.h>

/**
 * @brief Configuration error codes
 */
typedef enum {
    CFG_ERR_OK = 0,        ///< No error
    CFG_ERR_PARSE,         ///< Configuration parsing failed
    CFG_ERR_INVALID_VALUE, ///< Invalid configuration value
    CFG_ERR_NO_MEM,        ///< Not enough memory
    CFG_ERR_MAX,
} cfg_err_t;

/**
 * @brief Contains parsed configuration options
 */
typedef struct {
    const char *pid_file; ///< Path to PID file (default: "tmp/cweb.pid")
#ifdef CONFIG_DB
    const char *db_path; ///< Path to database file (default: "tmp/db")
    uint32_t db_size_mb; ///< Database size in megabytes (default: 64MB)
    uint32_t db_count;   ///< Number of named databases (default: 0)
#endif
#ifdef CONFIG_HTTP_SERVER
    const char *http_sock; ///< Path to HTTP server socket (default: "tmp/http.sock")
    const char *html_path; ///< Path to HTML files (default: "tmp/html")
#endif
#if defined(CONFIG_IPC_SERVER) || defined(CONFIG_IPC_CLIENT)
    const char *ipc_sock; ///< Path to IPC server socket (default: "tmp/ipc.sock")
#endif
#ifdef CONFIG_TELEBOT
    const char *bot_token; ///< Telegram bot token (default: NULL)
    uint32_t bot_upd_sec;  ///< Wait unil user interacts with bot (default: 30sec)
#endif
#ifdef CONFIG_LANG
    const char *lang_path; ///< Path to language files (default: "tmp/lang.json")
#endif
#ifdef CONFIG_DB_CRYPTO_TABLE
    const char *crypto_list_path; ///< Path to cryptocurrency list (default: "config/crypto-list.json")
#endif
#ifdef CONFIG_PARSER_CVBANKAS
    uint32_t parser_cvb_upd_sec; ///< CVBankas update interval in seconds (default: 5sec)
#endif
} cfg_t;

/**
 * @brief Parse configuration file
 * @param cfg - [out] Parsed configuration
 * @param cfg_file - [in] Configuration file path
 * @return CFG_ERR_OK on success, error code on failure
 */
cfg_err_t cfg_parse(cfg_t *cfg, const char *cfg_file);

/**
 * @brief Free configuration resources
 * @param cfg - [in] Configuration to free
 */
void cfg_free(void);
