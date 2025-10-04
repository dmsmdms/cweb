#include <global.h>
#include <assert.h>

static void signal_cb(struct ev_loop *loop, ev_signal *signal, int events)
{
    app_t *app = signal->data;
    app->is_running = false;
    log_info("get signal exit");
    ev_signal_stop(loop, &app->sigint);
    ev_signal_stop(loop, &app->sigterm);

    cvbankas_destroy(&app->cvbankas);
    http_destroy(&app->http);
}

int main(int argc, char *argv[])
{
    app_t app = {};

    app.loop = ev_default_loop(EVFLAG_SIGNALFD | EVFLAG_NOTIMERFD);
    ev_signal_init(&app.sigint, signal_cb, SIGINT);
    ev_signal_init(&app.sigterm, signal_cb, SIGTERM);
    app.sigint.data = &app;
    app.sigterm.data = &app;
    app.is_running = true;
    ev_signal_start(app.loop, &app.sigint);
    ev_signal_start(app.loop, &app.sigterm);

    assert(log_init(&app.log, NULL, LOG_LEVEL_DEBUG));
    http_init(&app.http);
    assert(db_open(&app.db, "build"));
    cvbankas_init(&app.cvbankas);

    ev_run(app.loop, 0);

    cvbankas_destroy(&app.cvbankas);
    db_close(&app.db);
    http_destroy(&app.http);
    log_destroy(&app.log);
    ev_loop_destroy(app.loop);

    return 0;
}
