#include <global.h>

static app_t *global_app = NULL;
static void sigkill_cb(int sig)
{
    app_t *app = global_app;
    log_error("get signal %s force exit", strsignal(sig));
#ifdef CONFIG_DB
    db_close(app); // We need correctly close DB to avoid corruption
#endif
    exit(EXIT_FAILURE);
}

static void sigterm_cb(struct ev_loop *loop, ev_signal *signal, int events)
{
    UNUSED(events);
    app_t *app = signal->data;
    app->is_running = false;
    log_info("get signal %s exit", strsignal(signal->signum));
    ev_signal_stop(loop, &app->sigint);
    ev_signal_stop(loop, &app->sigterm);
    // Stop all libev watchers for safe break from ev_run //
#ifdef CONFIG_PARSER_CVBANKAS
    parser_cvb_destroy(app);
#endif
#ifdef CONFIG_WS_CLIENT
    ws_client_destroy(app);
#endif
#ifdef CONFIG_HTTP_SERVER
    http_server_destroy(app);
#endif
#ifdef CONFIG_HTTP_CLIENT
    http_client_destroy(app);
#endif
}

static void app_init(app_t *app)
{
    global_app = app;
    signal(SIGKILL, sigkill_cb);
    signal(SIGSEGV, sigkill_cb);
    signal(SIGABRT, sigkill_cb);
    signal(SIGFPE, sigkill_cb);

    app->loop = ev_default_loop(EVFLAG_SIGNALFD | EVFLAG_NOTIMERFD);
    ev_signal_init(&app->sigint, sigterm_cb, SIGINT);
    ev_signal_init(&app->sigterm, sigterm_cb, SIGTERM);
    app->sigint.data = app;
    app->sigterm.data = app;
    app->is_running = true;
    ev_signal_start(app->loop, &app->sigint);
    ev_signal_start(app->loop, &app->sigterm);
}

static void cleanup(app_t *app)
{
#ifdef CONFIG_PARSER_BINANCE
    parser_binance_destroy(app);
#endif
#ifdef CONFIG_PARSER_CVBANKAS
    parser_cvb_destroy(app);
#endif
#ifdef CONFIG_DB
    db_close(app);
#endif
#ifdef CONFIG_WS_CLIENT
    ws_client_destroy(app);
#endif
#ifdef CONFIG_HTTP_SERVER
    http_server_destroy(app);
#endif
#ifdef CONFIG_HTTP_CLIENT
    http_client_destroy(app);
#endif
#ifdef CONFIG_LANG
    lang_free(app);
#endif
    mem_free(app);
    log_destroy(app);
    ev_loop_destroy(app->loop);
}

int main(int argc, char *argv[])
{
    app_t app = {};
    args_parse(&app, argc, argv);
    app_init(&app);

    if(!log_init(&app)) {
        cleanup(&app);
        return EXIT_FAILURE;
    }
    if(!mem_init(&app)) {
        cleanup(&app);
        return EXIT_FAILURE;
    }
    if(!cfg_parse(&app)) {
        cleanup(&app);
        return EXIT_FAILURE;
    }
#ifdef CONFIG_LANG
    if(!lang_load(&app)) {
        cleanup(&app);
        return EXIT_FAILURE;
    }
#endif
#ifdef CONFIG_HTTP_CLIENT
    if(!http_client_init(&app)) {
        cleanup(&app);
        return EXIT_FAILURE;
    }
#endif
#ifdef CONFIG_HTTP_SERVER
    if(!http_server_init(&app)) {
        cleanup(&app);
        return EXIT_FAILURE;
    }
#endif
#ifdef CONFIG_WS_CLIENT
    if(!ws_client_init(&app)) {
        cleanup(&app);
        return EXIT_FAILURE;
    }
#endif
#ifdef CONFIG_DB
    if(!db_open(&app)) {
        cleanup(&app);
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
    if(!parser_binance_init(&app)) {
        cleanup(&app);
        return EXIT_FAILURE;
    }
#endif
#ifdef CONFIG_APP_JOBLY_BOT
    if(!bot_jobly_init(&app)) {
        cleanup(&app);
        return EXIT_FAILURE;
    }
#endif
    db_crypto_export_csv_file(&app, "tmp/db.csv");

    ev_run(app.loop, 0);
    cleanup(&app);
    return EXIT_SUCCESS;
}
