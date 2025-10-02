#pragma once

#include <core/base/str.h>
#include <core/base/mem.h>
#include <core/base/args.h>
#include <core/base/file.h>
#include <core/base/config.h>
#include <core/json/json-parser.h>
#ifdef CONFIG_HTTP_CLIENT
    #include <core/http/http-client.h>
#endif
#ifdef CONFIG_HTTP_SERVER
    #include <core/http/http-parser-ext.h>
    #include <core/http/http-server.h>
    #include <core/http/http-gen.h>
    #include <core/html/html-gen.h>
#endif
#ifdef CONFIG_WS_CLIENT
    #include <core/ws/ws-client.h>
#endif
#ifdef CONFIG_LANG
    #include <core/lang.h>
#endif
#ifdef CONFIG_DB_JOB_TABLE
    #include <db/job.h>
#endif
#ifdef CONFIG_DB_CRYPTO_TABLE
    #include <db/db-crypto.h>
#endif
#ifdef CONFIG_PARSER_CVBANKAS
    #include <parser/cvbankas.h>
#endif
#ifdef CONFIG_PARSER_BINANCE
    #include <parser/parser-binance.h>
#endif
#ifdef CONFIG_APP_JOBLY_API
    #include <api/api-jobly.h>
#endif
#ifdef CONFIG_APP_CRYPTO_API
    #include <api/api-crypto.h>
#endif
#ifdef CONFIG_APP_JOBLY_BOT
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
#ifdef CONFIG_WS_CLIENT
    ws_client_t ws_client; ///< WebSocket client instance
#endif
#ifdef CONFIG_LANG
    lang_t lang; ///< Contains loaded language data
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
#ifdef CONFIG_PARSER_BINANCE
    parser_binance_t parser_binance; ///< Binance parser handler
#endif
#ifdef CONFIG_APP_JOBLY
    bot_jobly_t bot_jobly; ///< Jobly Telegram bot handler
#endif
    bool is_running; ///< Application running flag
} app_t;
