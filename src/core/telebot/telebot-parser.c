#include <core/telebot/telebot-parser.h>
#include <core/json/json-parser.h>
#include <core/base/log.h>
#include <core/base/buf.h>
#include <core/lang.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define MEM_BUF_SIZE (128 * 1024)

static const char *const chat_type_str[] = {
    [TELEBOT_CHAT_PRIVATE] = "private",
    [TELEBOT_CHAT_GROUP] = "group",
    [TELEBOT_CHAT_SUPERGROUP] = "supergroup",
    [TELEBOT_CHAT_CHANNEL] = "channel",
};
STATIC_ASSERT(ARRAY_SIZE(chat_type_str) == TELEBOT_CHAT_TYPE_MAX);
static const char *const entity_type_str[] = {
    [TELEBOT_ENTITY_MENTION] = "mention",
    [TELEBOT_ENTITY_HASHTAG] = "hashtag",
    [TELEBOT_ENTITY_CASHTAG] = "cashtag",
    [TELEBOT_ENTITY_BOT_COMMAND] = "bot_command",
    [TELEBOT_ENTITY_URL] = "url",
    [TELEBOT_ENTITY_EMAIL] = "email",
    [TELEBOT_ENTITY_PHONE_NUMBER] = "phone_number",
    [TELEBOT_ENTITY_BOLD] = "bold",
    [TELEBOT_ENTITY_ITALIC] = "italic",
    [TELEBOT_ENTITY_UNDERLINE] = "underline",
    [TELEBOT_ENTITY_STRIKETHROUGH] = "strikethrough",
    [TELEBOT_ENTITY_CODE] = "code",
    [TELEBOT_ENTITY_PRE] = "pre",
    [TELEBOT_ENTITY_TEXT_LINK] = "text_link",
    [TELEBOT_ENTITY_TEXT_MENTION] = "text_mention",
    [TELEBOT_ENTITY_CUSTOM_EMOJI] = "custom_emoji",
};
STATIC_ASSERT(ARRAY_SIZE(entity_type_str) == TELEBOT_ENTITY_TYPE_MAX);
static const char *const chat_member_status_str[] = {
    [TELEBOT_CHAT_MEMBER_STATUS_OWNER] = "creator", [TELEBOT_CHAT_MEMBER_STATUS_ADMIN] = "administrator",
    [TELEBOT_CHAT_MEMBER_STATUS_MEMBER] = "member", [TELEBOT_CHAT_MEMBER_STATUS_RESTRICTED] = "restricted",
    [TELEBOT_CHAT_MEMBER_STATUS_LEFT] = "left",     [TELEBOT_CHAT_MEMBER_STATUS_BANNED] = "kicked",
};
STATIC_ASSERT(ARRAY_SIZE(chat_member_status_str) == TELEBOT_CHAT_MEMBER_STATUS_MAX);
static buf_ext_t mem_buf = { 0 };

static json_parse_err_t parse_user(const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_user_t *user = priv_data;
    const char *lang_code = NULL;
    json_item_t items[] = {
        { "id", json_parse_int32, &user->id },
        { "first_name", json_parse_pstr, &user->first_name },
        { "last_name", json_parse_pstr, &user->last_name },
        { "username", json_parse_pstr, &user->username },
        { "language_code", json_parse_pstr, &lang_code },
        { "is_bot", json_parse_bool, &user->is_bot },
    };
    json_parse_err_t res = json_parse_obj(cur, json, items, ARRAY_SIZE(items));
    if(res != JSON_PARSE_ERR_OK) {
        return res;
    }
    if(lang_code) {
        user->language_code = LANG_CODE(lang_code[0], lang_code[1]);
    }
    return JSON_PARSE_ERR_OK;
}

static json_parse_err_t parse_chat(const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_chat_t *chat = priv_data;
    json_enum_t type_enum = {
        .pval = &chat->type,
        .enums = chat_type_str,
        .enums_count = ARRAY_SIZE(chat_type_str),
    };
    json_item_t items[] = {
        { "id", json_parse_int64, &chat->id },
        { "first_name", json_parse_pstr, &chat->first_name },
        { "last_name", json_parse_pstr, &chat->last_name },
        { "username", json_parse_pstr, &chat->username },
        { "type", json_parse_enum, &type_enum },
    };
    return json_parse_obj(cur, json, items, ARRAY_SIZE(items));
}

static json_parse_err_t parse_entity(const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_message_entity_t *entity = priv_data;
    json_enum_t type_enum = {
        .pval = &entity->type,
        .enums = entity_type_str,
        .enums_count = ARRAY_SIZE(entity_type_str),
    };
    json_item_t items[] = {
        { "offset", json_parse_int32, &entity->offset },
        { "length", json_parse_int32, &entity->length },
        { "type", json_parse_enum, &type_enum },
    };
    return json_parse_obj(cur, json, items, ARRAY_SIZE(items));
}

static json_parse_err_t parse_entities_arr(const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_message_entity_arr_t *entities = priv_data;
    return parse_entity(cur, json, &entities->data[entities->count++]);
}

static json_parse_err_t alloc_entities_arr(const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_message_entity_arr_t *entities = priv_data;
    entities->data = buf_calloc(&mem_buf, cur->size * sizeof(telebot_message_entity_t));
    if(entities->data == NULL) {
        log_error("no mem for entities[%u]", cur->size);
        return JSON_PARSE_ERR_NO_MEM;
    }
    return json_parse_arr(cur, json, parse_entities_arr, entities);
}

static json_parse_err_t parse_message(const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_message_t *msg = priv_data;
    json_item_t items[] = {
        { "message_id", json_parse_int32, &msg->message_id },
        { "from", parse_user, &msg->from },
        { "chat", parse_chat, &msg->chat },
        { "date", json_parse_int64, &msg->date },
        { "text", json_parse_pstr, &msg->text },
        { "entities", alloc_entities_arr, &msg->entities },
    };
    return json_parse_obj(cur, json, items, ARRAY_SIZE(items));
}

static json_parse_err_t parse_chat_member(const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_chat_member_t *member = priv_data;
    json_enum_t status_enum = {
        .pval = &member->status,
        .enums = chat_member_status_str,
        .enums_count = ARRAY_SIZE(chat_member_status_str),
    };
    json_item_t items[] = {
        { "status", json_parse_enum, &status_enum },
        { "user", parse_user, &member->user },
        { "until_date", json_parse_int64, &member->until_date },
    };
    return json_parse_obj(cur, json, items, ARRAY_SIZE(items));
}

static json_parse_err_t parse_chat_member_upd(const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_chat_member_update_t *upd = priv_data;
    json_item_t items[] = {
        { "chat", parse_chat, &upd->chat },
        { "from", parse_user, &upd->from },
        { "date", json_parse_int64, &upd->date },
        { "old_chat_member", parse_chat_member, &upd->old_chat_member },
        { "new_chat_member", parse_chat_member, &upd->new_chat_member },
    };
    return json_parse_obj(cur, json, items, ARRAY_SIZE(items));
}

static json_parse_err_t parse_message_upd(const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_update_t *update = priv_data;
    update->update_type = TELEBOT_UPDATE_MESSAGE;
    return parse_message(cur, json, &update->message);
}

static json_parse_err_t parse_my_chat_member_upd(const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_update_t *update = priv_data;
    update->update_type = TELEBOT_UPDATE_MY_CHAT_MEMBER;
    return parse_chat_member_upd(cur, json, &update->my_chat_member);
}

static json_parse_err_t parse_update(const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_update_t *update = priv_data;
    json_item_t items[] = {
        { "update_id", json_parse_int32, &update->update_id },
        { "message", parse_message_upd, update },
        { "my_chat_member", parse_my_chat_member_upd, update },
    };
    return json_parse_obj(cur, json, items, ARRAY_SIZE(items));
}

static json_parse_err_t parse_update_arr(const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_update_arr_t *updates = priv_data;
    return parse_update(cur, json, &updates->data[updates->count++]);
}

static json_parse_err_t alloc_update_arr(const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_update_arr_t *updates = priv_data;
    updates->data = buf_calloc(&mem_buf, cur->size * sizeof(telebot_update_t));
    if(updates->data == NULL) {
        log_error("no mem for updates[%u]", cur->size);
        return JSON_PARSE_ERR_NO_MEM;
    }
    return json_parse_arr(cur, json, parse_update_arr, updates);
}

static void parse_resp(const chttp_resp_t *http_resp, json_parse_cb_t result_cb)
{
    telebot_resp_t resp = { 0 };
    telebot_error_t *error = &resp.result.error;
    json_item_t items[] = {
        { "ok", json_parse_bool, &resp.ok },
        { "result", result_cb, &resp.result },
        { "error_code", json_parse_int32, &error->code },
        { "description", json_parse_pstr, &error->description },
    };
    char buf[MEM_BUF_SIZE];
    buf_init_ext(&mem_buf, buf, sizeof(buf));
    if(json_parse(http_resp->body.data, http_resp->body.len, items, ARRAY_SIZE(items)) != JSON_PARSE_ERR_OK) {
        log_debug("JSON: %s", http_resp->body.data);
        return;
    }
    const telebot_post_data_t *data = http_resp->user_data;
    if(data->cb) {
        data->cb(&resp);
    }
}

void telebot_get_updates_cb(const chttp_resp_t *http_resp)
{
    parse_resp(http_resp, alloc_update_arr);
}

void telebot_send_message_cb(const chttp_resp_t *http_resp)
{
    parse_resp(http_resp, NULL);
}
