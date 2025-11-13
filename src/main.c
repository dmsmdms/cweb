#include <core/telebot/telebot.h>
#include <core/base/daemon.h>
#include <core/base/args.h>
#include <core/base/log.h>
#include <core/base/cfg.h>
#include <core/ai/ai-gboost.h>
#include <core/http/http-server.h>
#include <core/http/http-client.h>
#include <core/ipc/ipc-server.h>
#include <core/ipc/ipc-client.h>
#include <core/ws/ws-client.h>
#include <core/db/db.h>
#include <core/lang.h>
#include <db/db-crypto.h>
#include <parser/parser-binance.h>
#include <ipc/ipc-crypto-parser-server.h>
#include <ipc/ipc-crypto-parser-client.h>
#include <bot/bot-crypto-notify.h>
#include <bot/bot-admin.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ev.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

typedef struct {
    ev_signal sigint;
    ev_signal sigterm;
} main_t;

bool app_is_running = true;

static void sigkill_cb(int sig)
{
    log_error("get signal %s force exit", strsignal(sig));
#ifdef CONFIG_DB
    db_close(); // We need correctly close DB to avoid corruption
#endif
    exit(EXIT_FAILURE);
}

static void sigterm_cb(struct ev_loop *loop, ev_signal *signal, UNUSED int events)
{
    main_t *main = signal->data;
    ev_signal_stop(loop, &main->sigint);
    ev_signal_stop(loop, &main->sigterm);
    app_is_running = false;

    log_info("get signal %s exit", strsignal(signal->signum));

    // Stop all libev watchers for safe break from ev_run //
#ifdef CONFIG_AI_CRYPTO_TRAIN
    db_crypto_ai_deinit();
#endif
#ifdef CONFIG_APP_BOT_ADMIN
    bot_admin_deinit();
#endif
#ifdef CONFIG_PARSER_CVBANKAS
    parser_cvb_destroy();
#endif
#ifdef CONFIG_WS_CLIENT
    cws_destroy();
#endif
#ifdef CONFIG_IPC_CRYPTO_PARSER_CLIENT
    cipc_crypto_parser_deinit();
#endif
#ifdef CONFIG_IPC_SERVER
    sipc_deinit();
#endif
#ifdef CONFIG_HTTP_SERVER
    shttp_destroy();
#endif
#ifdef CONFIG_HTTP_CLIENT
    chttp_destroy();
#endif
}

static void main_init(main_t *main)
{
    signal(SIGKILL, sigkill_cb);
    signal(SIGSEGV, sigkill_cb);
    signal(SIGABRT, sigkill_cb);
    signal(SIGFPE, sigkill_cb);
    signal(SIGPIPE, SIG_IGN);

    ev_default_loop(EVFLAG_SIGNALFD | EVFLAG_NOTIMERFD);
    ev_signal_init(&main->sigint, sigterm_cb, SIGINT);
    ev_signal_init(&main->sigterm, sigterm_cb, SIGTERM);
    main->sigint.data = main;
    main->sigterm.data = main;
    ev_signal_start(EV_DEFAULT, &main->sigint);
    ev_signal_start(EV_DEFAULT, &main->sigterm);
}

#ifdef CONFIG_DB
static int db_import(const char *table, UNUSED const char *prm, UNUSED const char *file)
{
    #ifdef CONFIG_DB_CRYPTO_TABLE
    if(strcmp(table, "crypto") == 0) {
        if(db_crypto_import_csv(file, prm) != DB_ERR_OK) {
            return EXIT_FAILURE;
        }
    } else
    #endif
    {
        log_error("unknown table for import: %s", table);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int db_export(const char *table, UNUSED const char *prm, UNUSED const char *file)
{
    #ifdef CONFIG_DB_CRYPTO_TABLE
    if(strcmp(table, "crypto") == 0) {
        if(db_crypto_export_csv(file, prm) != DB_ERR_OK) {
            return EXIT_FAILURE;
        }
    } else
    #endif
    #ifdef CONFIG_CALC_CRYPTO
            if(strcmp(table, "crypto-calc") == 0) {
        if(db_crypto_export_calc_csv(file, prm) != DB_ERR_OK) {
            return EXIT_FAILURE;
        }
    } else
    #endif
    {
        log_error("unknown table for export: %s", table);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
#endif

static void cleanup(void)
{
#ifdef CONFIG_APP_BOT_ADMIN
    bot_admin_deinit();
#endif
#ifdef CONFIG_PARSER_BINANCE
    parser_bin_destroy();
#endif
#ifdef CONFIG_PARSER_CVBANKAS
    parser_cvb_destroy();
#endif
#ifdef CONFIG_AI_GBOOST
    ai_gb_deinit();
#endif
#ifdef CONFIG_DB
    db_close();
#endif
#ifdef CONFIG_WS_CLIENT
    cws_destroy();
#endif
#ifdef CONFIG_IPC_CRYPTO_PARSER_CLIENT
    cipc_crypto_parser_deinit();
#endif
#ifdef CONFIG_IPC_SERVER
    sipc_deinit();
#endif
#ifdef CONFIG_HTTP_SERVER
    shttp_destroy();
#endif
#ifdef CONFIG_HTTP_CLIENT
    chttp_destroy();
#endif
#ifdef CONFIG_LANG
    lang_free();
#endif
    log_destroy();
    daemon_cleanup();
    cfg_free();
    ev_loop_destroy(EV_DEFAULT);
}

int main(int argc, char *argv[])
{
    main_t main;
    main_init(&main);
    args_t args;
    if(args_parse(&args, argc, argv) != ARGS_ERR_OK) {
        cleanup();
        return EXIT_FAILURE;
    }
    cfg_t cfg;
    if(cfg_parse(&cfg, args.cfg_file) != CFG_ERR_OK) {
        cleanup();
        return EXIT_FAILURE;
    }
    if(args.run_daemon) {
        daemon_err_t res = daemon_init(cfg.pid_file);
        if(res != DAEMON_ERR_OK) {
            cleanup();
            return (res == DAEMON_ERR_PARENT) ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    } else if(args.stop_daemon) {
        int ret = EXIT_SUCCESS;
        if(daemon_stop(cfg.pid_file) != DAEMON_ERR_OK) {
            ret = EXIT_FAILURE;
        }
        cleanup();
        return ret;
    }
    if(log_init(args.log_lvl, args.log_file) != LOG_ERR_OK) {
        cleanup();
        return EXIT_FAILURE;
    }
#ifdef CONFIG_DB
    const char *model = NULL;
    #ifdef CONFIG_AI_CRYPTO_TRAIN
    model = getenv("AI_MODEL");
    #endif
    bool db_rd_only = args.db_export_file || model ? true : false;
    if(db_open(cfg.db_path, cfg.db_size_mb, cfg.db_count, db_rd_only) != DB_ERR_OK) {
        cleanup();
        return EXIT_FAILURE;
    }
#endif
#ifdef CONFIG_DB_CRYPTO_TABLE
    if(db_crypto_init(cfg.crypto_list_path) != DB_ERR_OK) {
        cleanup();
        return EXIT_FAILURE;
    }
#endif
#ifdef CONFIG_DB
    if(args.db_import_file) {
        int res = db_import(args.db_table, args.db_prm, args.db_import_file);
        cleanup();
        return res;
    }
    if(args.db_export_file) {
        int res = db_export(args.db_table, args.db_prm, args.db_export_file);
        cleanup();
        return res;
    }
#endif
#ifdef CONFIG_LANG
    if(lang_load(cfg.lang_path) != LANG_ERR_OK) {
        cleanup();
        return EXIT_FAILURE;
    }
#endif
#ifdef CONFIG_HTTP_CLIENT
    if(chttp_init() != CHTTP_ERR_OK) {
        cleanup();
        return EXIT_FAILURE;
    }
#endif
#ifdef CONFIG_HTTP_SERVER
    if(shttp_init(cfg.http_sock) != SHTTP_ERR_OK) {
        cleanup();
        return EXIT_FAILURE;
    }
#endif
#ifdef CONFIG_IPC_CRYPTO_PARSER_SERVER
    if(sipc_crypto_parser_init(cfg.ipc_sock) != SIPC_ERR_OK) {
        cleanup();
        return EXIT_FAILURE;
    }
#endif
#ifdef CONFIG_IPC_CRYPTO_PARSER_CLIENT
    if(cipc_crypto_parser_init(cfg.ipc_sock) != CIPC_ERR_OK) {
        cleanup();
        return EXIT_FAILURE;
    }
#endif
#ifdef CONFIG_WS_CLIENT
    if(cws_init() != CWS_ERR_OK) {
        cleanup();
        return EXIT_FAILURE;
    }
#endif
#ifdef CONFIG_HTML_PARSER
    html_parser_init(&app);
#endif
#ifdef CONFIG_PARSER_CVBANKAS
    parser_cvb_init(&app);
#endif
#ifdef CONFIG_PARSER_BINANCE
    if(parser_bin_init() != PARSER_BIN_ERR_OK) {
        cleanup();
        return EXIT_FAILURE;
    }
#endif
#ifdef CONFIG_APP_CRYPTO_BOT_NOTIFY
    if(bot_crypto_notify_init(cfg.bot_token, cfg.bot_upd_sec) != BOT_CRYPTO_ERR_OK) {
        cleanup();
        return EXIT_FAILURE;
    }
#endif
#ifdef CONFIG_APP_BOT_ADMIN
    if(bot_admin_init(cfg.bot_token, cfg.bot_upd_sec) != BOT_ADMIN_ERR_OK) {
        cleanup();
        return EXIT_FAILURE;
    }
#endif
#ifdef CONFIG_AI_CRYPTO_TRAIN
    if(model) {
        char path[1024];
        sprintf(path, "model/%s.ubj", model);
        if(access("model", F_OK) != 0) {
            mkdir("model", 0755);
        }
        if(db_crypto_ai_train_model(path, model) != DB_ERR_OK) {
            cleanup();
            return EXIT_FAILURE;
        }
    }
#endif

    ev_run(EV_DEFAULT, 0);
    cleanup();

    return EXIT_SUCCESS;
}
