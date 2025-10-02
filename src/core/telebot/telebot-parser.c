#include <global.h>

static const char *const chat_type_str[] = {
    [TELEBOT_CHAT_PRIVATE] = "private",
    [TELEBOT_CHAT_GROUP] = "group",
    [TELEBOT_CHAT_SUPERGROUP] = "supergroup",
    [TELEBOT_CHAT_CHANNEL] = "channel",
};
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
};

static bool parse_user(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_user_t *user = priv_data;
    json_item_t items[] = {
        { "id", json_parse_int32, &user->id },
        { "first_name", json_parse_pstr, &user->first_name },
        { "last_name", json_parse_pstr, &user->last_name },
        { "username", json_parse_pstr, &user->username },
        { "language_code", json_parse_pstr, &user->language_code },
        { "is_bot", json_parse_bool, &user->is_bot },
    };
    return json_parse_obj(app, cur, json, items, ARRAY_SIZE(items));
}

static bool parse_chat(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
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
    return json_parse_obj(app, cur, json, items, ARRAY_SIZE(items));
}

static bool parse_entity(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
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
    return json_parse_obj(app, cur, json, items, ARRAY_SIZE(items));
}

static bool parse_entities_arr(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_message_entity_arr_t *entities = priv_data;
    if(entities->count >= TELEBOT_MAX_ENTITIES) {
        log_error("too many entities");
        return false;
    }
    if(!parse_entity(app, cur, json, &entities->data[entities->count])) {
        return false;
    }
    entities->count++;
    return true;
}

static bool alloc_entities_arr(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_message_entity_arr_t *entities = priv_data;
    entities->data = mem_calloc(app, __func__, TELEBOT_MAX_ENTITIES * sizeof(telebot_message_entity_t));
    if(entities->data == NULL) {
        return false;
    }
    return json_parse_arr(app, cur, json, parse_entities_arr, entities);
}

static bool parse_message(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
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
    return json_parse_obj(app, cur, json, items, ARRAY_SIZE(items));
}

static bool parse_update(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_update_t *update = priv_data;
    json_item_t items[] = {
        { "update_id", json_parse_int32, &update->update_id },
        { "message", parse_message, &update->message },
    };
    return json_parse_obj(app, cur, json, items, ARRAY_SIZE(items));
}

static bool parse_update_arr(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_update_arr_t *updates = priv_data;
    if(updates->count >= TELEBOT_MAX_UPDATES) {
        log_error("too many updates");
        return false;
    }
    if(!parse_update(app, cur, json, &updates->data[updates->count])) {
        return false;
    }
    updates->count++;
    return true;
}

static bool alloc_update_arr(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    telebot_update_arr_t *updates = priv_data;
    updates->data = mem_calloc(app, __func__, TELEBOT_MAX_UPDATES * sizeof(telebot_update_t));
    if(updates->data == NULL) {
        return false;
    }
    return json_parse_arr(app, cur, json, parse_update_arr, updates);
}

static void parse_resp(const http_resp_t *http_resp, json_parse_cb_t result_cb)
{
    app_t *app = http_resp->app;
    telebot_resp_t resp = {
        .app = app,
    };
    json_item_t items[] = {
        { "ok", json_parse_bool, &resp.ok },
        { "result", result_cb, &resp.result },
    };
    uint32_t mem_offset = mem_get_offset(app);
    if(json_parse_wrap(app, http_resp->data, http_resp->size, items, ARRAY_SIZE(items))) {
        const telebot_post_data_t *data = http_resp->priv_data;
        if(data->cb) {
            data->cb(&resp);
        }
    }
    mem_put_offset(app, mem_offset);
}

void telebot_get_updates_cb(const http_resp_t *http_resp)
{
    parse_resp(http_resp, alloc_update_arr);
}
