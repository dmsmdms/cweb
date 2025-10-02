#include <global.h>

#define LANG_MAX_DATA_SIZE   (1024 * 1024)
#define LANG_MAX_ASSET_COUNT (512)

static bool parse_asset_str(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    lang_t *lang = &app->lang;
    size_t str_len = cur->end - cur->start + 1;
    if((lang->data_size + str_len) > LANG_MAX_DATA_SIZE) {
        log_error("lang data overflow");
        return false;
    }
    if(!json_parse_pstr(app, cur, json, priv_data)) {
        return false;
    }
    log_debug("parse asset %.*s", cur->end - cur->start, json + cur->start);
    lang->data_size += str_len;
    return true;
}

static bool parse_asset(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    lang_asset_t *asset = priv_data;
    json_item_t items[] = {
        { "en", parse_asset_str, &asset->str[LANG_EN] },
        { "ru", parse_asset_str, &asset->str[LANG_RU] },
        { "lt", parse_asset_str, &asset->str[LANG_LT] },
        { "uk", parse_asset_str, &asset->str[LANG_UK] },
    };
    return json_parse_obj(app, cur, json, items, ARRAY_SIZE(items));
}

static bool parse_asset_arr(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data)
{
    UNUSED(priv_data);
    lang_t *lang = &app->lang;
    if(lang->assets_count >= LANG_MAX_ASSET_COUNT) {
        log_error("too many assets");
        return false;
    }
    return parse_asset(app, cur, json, &lang->assets[lang->assets_count++]);
}

static bool lang_parse_cb(app_t *app, const char *data, uint32_t size, void *priv_data)
{
    UNUSED(priv_data);
    lang_t *lang = &app->lang;
    lang->data = mem_alloc(app, __func__, LANG_MAX_DATA_SIZE);
    if(lang->data == NULL) {
        return false;
    }
    lang->assets = mem_calloc(app, __func__, LANG_MAX_ASSET_COUNT * sizeof(lang_asset_t));
    if(lang->assets == NULL) {
        return false;
    }
    json_item_t items[] = {
        { NULL, parse_asset_arr, NULL },
    };
    return json_parse_wrap(app, data, size, items, ARRAY_SIZE(items));
}

bool lang_load(app_t *app)
{
    const config_t *cfg = &app->cfg;
    uint32_t mem_offset = mem_get_offset(app);
    bool res = file_read(app, cfg->lang_path, lang_parse_cb, NULL);
    if(res == false) {
        log_error("read lang %s failed", cfg->lang_path);
    }
    mem_put_offset(app, mem_offset);
    return res;
}

void lang_free(app_t *app)
{
    UNUSED(app);
}
