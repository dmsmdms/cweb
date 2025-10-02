#pragma once

#include <core/base/str.h>
#include <core/base/args.h>
#include <core/base/file.h>
#include <core/base/config.h>
#include <core/json/json-parser.h>
#ifdef CONFIG_HTTP_CLIENT
    #include <core/http/http-client.h>
#endif
#ifdef CONFIG_HTTP_SERVER
    #include <core/http/http-server.h>
    #include <core/html/html-gen.h>
#endif
#ifdef CONFIG_DB_JOB_TABLE
    #include <db/job.h>
#endif
#ifdef CONFIG_PARSER_CVBANKAS
    #include <parser/cvbankas.h>
#endif
#ifdef CONFIG_APP_JOBLY
    #include <backend/backend-jobly.h>
    #include <bot/bot-jobly.h>
#endif

/**
 * @brief Global application state structure
 */
typedef struct app {
    struct ev_loop *loop; ///< Event loop
    ev_signal sigint;     ///< SIGINT handler
    ev_signal sigterm;    ///< SIGTERM handler
    args_t args;          ///< Command line arguments
    log_t log;            ///< Logging handler
    config_t cfg;         ///< Configuration variables
    mem_t mem;            ///< Temporary memory buffer
#ifdef CONFIG_HTTP_CLIENT
    http_client_t http_client; ///< HTTP multi-connection cleint
#endif
#ifdef CONFIG_HTTP_SERVER
    http_server_t http_server; ///< HTTP server instance
#endif
#ifdef CONFIG_DB
    db_t db; ///< Database handler
#endif
#ifdef CONFIG_HTML_PARSER
    html_parser_t html_parser; ///< HTML parser with temporary buffer
#endif
#ifdef CONFIG_PARSER_CVBANKAS
    parser_cvb_t parser_cvb; ///< CVBankas parser handler
#endif
#ifdef CONFIG_APP_JOBLY
    bot_jobly_t bot_jobly; ///< Jobly Telegram bot handler
#endif
    bool is_running; ///< Application running flag
} app_t;
