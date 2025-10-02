#include <core/telebot/telebot-method.h>
#include <core/telebot/telebot-parser.h>
#include <core/http/http-client-mime.h>
#include <stdio.h>

#define URL_SIZE 512

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
static const char *token = NULL;

void telebot_set_token(const char *tok)
{
    token = tok;
}

static telebot_err_t telebot_post(const char *method, const chttp_mime_gen_t *mimes, uint32_t mime_count,
                                  chttp_resp_cb_t http_cb, telebot_resp_cb_t telebot_cb)
{
    char url[URL_SIZE];
    snprintf(url, sizeof(url), "https://api.telegram.org/bot%s/%s", token, method);
    telebot_post_data_t data = {
        .cb = telebot_cb,
    };
    buf_t user_data = {
        .data = &data,
        .size = sizeof(data),
    };
    if(chttp_post_gen(url, mimes, mime_count, http_cb, &user_data) != CHTTP_ERR_OK) {
        return TELEBOT_ERR_HTTP;
    }
    return TELEBOT_ERR_OK;
}

telebot_err_t telebot_get_updates(uint32_t offset, uint32_t limit, uint32_t timeout,
                                  const telebot_update_type_t *allowed_updates, uint32_t allowed_updates_count,
                                  telebot_resp_cb_t cb)
{
    chttp_mime_gen_t mimes[4] = {
        CHTTP_MIME_GEN_UINT32("offset", offset),
        CHTTP_MIME_GEN_UINT32("limit", limit),
        CHTTP_MIME_GEN_UINT32("timeout", timeout),
    };
    uint32_t mimes_count = 3;
    if(allowed_updates) {
        chttp_mime_enum_arr_t arr = {
            .arr = allowed_updates,
            .count = allowed_updates_count,
            .enums = update_type_str,
        };
        mimes[mimes_count++] = CHTTP_MIME_GEN_JENUM_ARR("allowed_updates", &arr);
    }
    return telebot_post("getUpdates", mimes, mimes_count, telebot_get_updates_cb, cb);
}

telebot_err_t telebot_send_message(uint64_t chat_id, const char *text)
{
    chttp_mime_gen_t mimes[2] = {
        CHTTP_MIME_GEN_UINT64("chat_id", chat_id),
        CHTTP_MIME_GEN_STR("text", text),
    };
    return telebot_post("sendMessage", mimes, ARRAY_SIZE(mimes), telebot_send_message_cb, NULL);
}
