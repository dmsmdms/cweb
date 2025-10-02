#include <global.h>

static bool get_updates(app_t *app);
static void get_updates_cb(const telebot_resp_t *resp)
{
    app_t *app = resp->app;
    get_updates(app);
}

static bool get_updates(app_t *app)
{
    bot_jobly_t *jobly = &app->bot_jobly;
    const config_t *cfg = &app->cfg;
    return telebot_get_updates(&jobly->bot, 0, TELEBOT_MAX_UPDATES, cfg->bot_jobly_upd_sec, NULL, 0, get_updates_cb);
}

bool bot_jobly_init(app_t *app)
{
    bot_jobly_t *jobly = &app->bot_jobly;
    const config_t *cfg = &app->cfg;
    if(cfg->bot_jobly_token[0] == '\0') {
        log_error("token is not set");
        return false;
    }
    telebot_init(app, &jobly->bot, cfg->bot_jobly_token);
    return get_updates(app);
}
