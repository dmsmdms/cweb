#include <global.h>

static const char *const update_type_str[] = {
    [TELEBOT_UPDATE_MESSAGE] = "message",
    [TELEBOT_UPDATE_EDITED_MESSAGE] = "edited_message",
    [TELEBOT_UPDATE_CHANNEL_POST] = "channel_post",
    [TELEBOT_UPDATE_EDITED_CHANNEL_POST] = "edited_channel_post",
    [TELEBOT_UPDATE_INLINE_QUERY] = "inline_query",
    [TELEBOT_UPDATE_CHOSEN_INLINE_RESULT] = "chonse_inline_result",
    [TELEBOT_UPDATE_CALLBACK_QUERY] = "callback_query",
    [TELEBOT_UPDATE_SHIPPING_QUERY] = "shipping_query",
    [TELEBOT_UPDATE_PRE_CHECKOUT_QUERY] = "pre_checkout_query",
    [TELEBOT_UPDATE_POLL] = "poll",
    [TELEBOT_UPDATE_POLL_ANSWER] = "poll_answer",
};

static void mime_push_int(http_mime_t *mime, uint32_t *mime_count, char *buf, const char *name, int num)
{
    sprintf(buf, "%d", num);
    mime->type = HTTP_MIME_TYPE_DATA;
    mime->name = name;
    mime->data = buf;
    (*mime_count)++;
}

static void mime_push_str(http_mime_t *mime, uint32_t *mime_count, const char *name, const char *str)
{
    mime->type = HTTP_MIME_TYPE_DATA;
    mime->name = name;
    mime->data = str;
    (*mime_count)++;
}

static void enum_to_json(char *buf, const char *const enum_to_str[], const uint32_t *enums, uint32_t enums_count)
{
    buf += sprintf(buf, "[,");
    for(uint32_t i = 0; i < enums_count; i++) {
        uint32_t val = enums[i];
        buf += sprintf(buf, "\"%s\",", enum_to_str[val]);
    }
    sprintf(buf - 1, "]");
}

static bool telebot_post(telebot_t *bot, const char *method, const http_mime_t *mimes, uint32_t mime_count,
                         http_resp_cb_t http_cb, telebot_resp_cb_t telebot_cb)
{
    char url[512];
    sprintf(url, "https://api.telegram.org/bot%s/%s", bot->token, method);
    telebot_post_data_t data = {
        .cb = telebot_cb,
    };
    return http_post(bot->app, url, mimes, mime_count, http_cb, &data, sizeof(data));
}

void telebot_init(app_t *app, telebot_t *bot, const char *token)
{
    bot->token = token;
    bot->app = app;
}

bool telebot_get_updates(telebot_t *bot, uint32_t offset, uint32_t limit, uint32_t timeout,
                         const telebot_update_type_t *allowed_updates, uint32_t allowed_updates_count,
                         telebot_resp_cb_t cb)
{
    uint32_t index = 0;
    http_mime_t mimes[4];
    char offset_buf[16];
    mime_push_int(&mimes[index], &index, offset_buf, "offset", offset);
    char limit_buf[16];
    mime_push_int(&mimes[index], &index, limit_buf, "limit", limit);
    char timeout_buf[16];
    mime_push_int(&mimes[index], &index, timeout_buf, "timeout", timeout);
    if(allowed_updates) {
        char allowed_upd_buf[256];
        enum_to_json(allowed_upd_buf, update_type_str, allowed_updates, allowed_updates_count);
        mime_push_str(&mimes[index], &index, "allowed_updates", allowed_upd_buf);
    }
    return telebot_post(bot, "getUpdates", mimes, index, telebot_get_updates_cb, cb);
}
