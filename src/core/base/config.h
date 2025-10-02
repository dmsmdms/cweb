#pragma once

#include <common.h>

/**
 * @brief Contains parsed configuration options
 */
typedef struct {
#ifdef CONFIG_DB
    char db_path[256];   ///< Path to database file (default: "tmp/main.db")
    uint32_t db_size_mb; ///< Database size in megabytes (default: 64MB)
#endif
#ifdef CONFIG_HTTP_SERVER
    char http_sock[256]; ///< Path to HTTP server socket (default: "tmp/main.sock")
#endif
#ifdef CONFIG_PARSER_CVBANKAS
    uint32_t parser_cvb_upd_sec; ///< CVBankas update interval in seconds (default: 5sec)
#endif
#ifdef CONFIG_APP_JOBLY
    char html_jobly_path[256];  ///< Path to Jobly HTML files (default: "html/jobly")
    char bot_jobly_token[64];   ///< Jobly bot token (default: NULL)
    uint32_t bot_jobly_upd_sec; ///< Wait unil user interacts with bot (default: 30sec)
#endif
} config_t;

/**
 * @brief Parse configuration file
 * @param app - [in] Pointer to the application structure
 * @return true on success, false otherwise
 */
bool cfg_parse(app_t *app);
