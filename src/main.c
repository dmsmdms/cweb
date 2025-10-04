#include <global.h>
#include <assert.h>

int main(int argc, char *argv[])
{
    app_t app = {};
    app.loop = ev_default_loop(EVFLAG_SIGNALFD | EVFLAG_NOTIMERFD);
    assert(log_init(&app.log, NULL, LOG_LEVEL_DEBUG));
    http_init(&app.http);
    assert(db_open(&app.db, "build"));
    cvbankas_init(&app.cvbankas);

    ev_run(app.loop, 0);

    cvbankas_destroy(&app.cvbankas);
    db_close(&app.db);
    http_destroy(&app.http);
    log_destroy(&app.log);
    return 0;
}
