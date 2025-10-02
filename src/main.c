#include <global.h>
#include <assert.h>

int main(int argc, char *argv[])
{
    app_t app = {};
    assert(ev_init(&app.loop));
    assert(log_init(&app.log, NULL, LOG_LEVEL_DEBUG));
    http_init(&app.http);
    assert(db_open(&app.db, "build"));
    cvbankas_init(&app.cvbankas);

    ev_run(&app.loop);

    cvbankas_destroy(&app.cvbankas);
    db_close(&app.db);
    http_destroy(&app.http);
    log_destroy(&app.log);
    return 0;
}
