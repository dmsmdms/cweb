#include <core/lang.h>
#include <core/json/json-parser.h>
#include <core/base/file.h>
#include <core/base/buf.h>
#include <core/base/log.h>
#include <search.h>
#include <string.h>
#include <malloc.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define LANG_BUF_SIZE (128 * 1024)

typedef enum {
    LANG_ID_EN,
    LANG_ID_RU,
    LANG_ID_LT,
    LANG_ID_UK,
    LANG_ID_MAX,
} lang_id_t;

typedef struct {
    str_buf_t key;
    str_buf_t str[LANG_ID_MAX];
} lang_gen_asset_t;

typedef struct {
    char *key;
    char *str[LANG_ID_MAX];
} lang_asset_t;

typedef struct {
    lang_gen_asset_t *assets;
    uint32_t count;
    uint32_t offset;
    buf_ext_t buf;
} lang_gen_t;

typedef struct {
    struct hsearch_data htab;
    lang_asset_t *assets;
    uint32_t count;
} lang_t;

static lang_t lang = {};

static lang_id_t lang_get_id(lang_code_t lang_code)
{
    switch(lang_code) {
    case LANG_CODE_EN:
        return LANG_ID_EN;
    case LANG_CODE_RU:
        return LANG_ID_RU;
    case LANG_CODE_LT:
        return LANG_ID_LT;
    case LANG_CODE_UK:
        return LANG_ID_UK;
    default:
        log_warn("unknown lang_code %c%c, fallback to EN", lang_code >> 8, lang_code & 0xFF);
        return LANG_ID_EN;
    }
}

static json_parse_err_t json_parse_asset(const jsmntok_t *cur, const char *json, void *priv_data)
{
    lang_gen_t *gen = priv_data;
    lang_gen_asset_t *asset = &gen->assets[gen->count++];
    json_item_t items[] = {
        { "en", json_parse_pstr, &asset->str[LANG_ID_EN].data },
        { "ru", json_parse_pstr, &asset->str[LANG_ID_RU].data },
        { "lt", json_parse_pstr, &asset->str[LANG_ID_LT].data },
        { "uk", json_parse_pstr, &asset->str[LANG_ID_UK].data },
    };
    json_parse_err_t res = json_parse_obj(cur, json, items, ARRAY_SIZE(items));
    if(res != JSON_PARSE_ERR_OK) {
        return res;
    }

    // Key //
    const jsmntok_t *key = &cur[-1];
    if(key->type != JSMN_STRING) {
        log_error("invalid asset key type %d", key->type);
        return JSON_PARSE_ERR_INVALID;
    }
    asset->key.data = (char *)&json[key->start];
    asset->key.size = key->end - key->start;
    asset->key.data[asset->key.size++] = '\0';
    asset->key.offset = gen->offset;
    gen->offset += asset->key.size;

    // Strings //
    for(uint32_t i = 0; i < LANG_ID_MAX; i++) {
        str_buf_t *str = &asset->str[i];
        if(str->data == NULL) {
            log_error("no asset %s for lang_id %u", asset->key.data, i);
            return JSON_PARSE_ERR_CONVERT;
        }
        str->size = strlen(str->data) + 1;
        str->offset = gen->offset;
        gen->offset += str->size;
    }
    return JSON_PARSE_ERR_OK;
}

static json_parse_err_t json_parse_lang_arr(const jsmntok_t *cur, const char *json, void *priv_data)
{
    lang_gen_t *gen = priv_data;
    gen->assets = buf_calloc(&gen->buf, cur->size * sizeof(lang_gen_asset_t));
    if(gen->assets == NULL) {
        return JSON_PARSE_ERR_NO_MEM;
    }
    gen->offset = cur->size * sizeof(lang_gen_asset_t);
    json_item_t items[] = {
        { NULL, json_parse_asset, gen },
    };
    return json_parse_obj(cur, json, items, ARRAY_SIZE(items));
}
lang_err_t lang_load(const char *lang_file)
{
    // Read language file //
    char buf[LANG_BUF_SIZE];
    str_t file = {
        .data = buf,
        .len = sizeof(buf),
    };
    if(file_read(lang_file, &file) != FILE_ERR_OK) {
        return LANG_ERR_LOAD;
    }

    // Parse JSON //
    char json_buf[LANG_BUF_SIZE];
    lang_gen_t gen = {
        .buf.data = json_buf,
        .buf.size = sizeof(json_buf),
    };
    json_item_t items[] = {
        { "lang", json_parse_lang_arr, &gen },
    };
    if(json_parse(file.data, file.len, items, ARRAY_SIZE(items)) != JSON_PARSE_ERR_OK) {
        return LANG_ERR_PARSE;
    }

    // Allocate assets and hashtable //
    lang.assets = malloc(gen.offset);
    if(lang.assets == NULL) {
        log_error("malloc(%u) assets failed", gen.offset);
        return LANG_ERR_NO_MEM;
    }
    if(hcreate_r(2 * gen.count, &lang.htab) < 0) {
        log_error("htab[%u] create failed", 2 * gen.count);
        lang_free();
        return LANG_ERR_NO_MEM;
    }
    lang.count = gen.count;

    // Store parsed data //
    for(uint32_t i = 0; i < gen.count; i++) {
        lang_gen_asset_t *src = &gen.assets[i];
        lang_asset_t *dst = &lang.assets[i];
        dst->key = (char *)lang.assets + src->key.offset;
        memcpy(dst->key, src->key.data, src->key.size);

        for(uint32_t j = 0; j < LANG_ID_MAX; j++) {
            str_buf_t *src_str = &src->str[j];
            dst->str[j] = (char *)lang.assets + src_str->offset;
            memcpy(dst->str[j], src_str->data, src_str->size);
        }

        ENTRY *e_ptr, entry = {
            .key = dst->key,
            .data = dst,
        };
        if(hsearch_r(entry, ENTER, &e_ptr, &lang.htab) < 0) {
            log_error("htab insert failed for %s", dst->key);
            lang_free();
            return LANG_ERR_NO_MEM;
        }
    }

    return LANG_ERR_OK;
}

void lang_free(void)
{
    hdestroy_r(&lang.htab);
    lang.htab.table = NULL;
    free(lang.assets);
    lang.assets = NULL;
}

const char *lang_get_str(lang_code_t lang_code, const char *key)
{
    ENTRY *e_ptr, entry = {
        .key = (char *)key,
    };
    if(hsearch_r(entry, FIND, &e_ptr, &lang.htab) < 0) {
        log_warn("lang key %s not found", key);
        return NULL;
    }

    lang_id_t lang_id = lang_get_id(lang_code);
    const lang_asset_t *asset = e_ptr->data;
    return asset->str[lang_id];
}
