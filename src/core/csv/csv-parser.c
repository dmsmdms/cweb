#include <core/csv/csv-parser.h>
#include <core/csv/minicsv.h>
#include <core/base/file.h>
#include <core/base/log.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define MAX_CSV_COLS 128

typedef struct csv_parse_ctx {
    const char *const *col_names;
    uint32_t col_idx[MAX_CSV_COLS];
    uint32_t col_count;
    csv_parse_err_t res;
    csv_row_cb_t row_cb;
    void *priv_data;
    bool head_parsed;
} csv_parse_ctx_t;

static csv_parse_err_t parse_head(csv_parse_ctx_t *ctx, const char **cols, const uint32_t cols_count)
{
    for(uint32_t i = 0; i < cols_count; i++) {
        for(uint32_t j = 0; j < ctx->col_count; j++) {
            if(strcmp(cols[i], ctx->col_names[j]) == 0) {
                ctx->col_idx[j] = i;
                break;
            }
        }
    }
    for(uint32_t j = 0; j < ctx->col_count; j++) {
        if(ctx->col_idx[j] == UINT32_MAX) {
            log_error("CSV column '%s' missing", ctx->col_names[j]);
            return CSV_PARSE_ERR_INVALID;
        }
    }
    return CSV_PARSE_ERR_OK;
}

static int32_t stream_cb(char *data, uint32_t len, void *priv_data)
{
    csv_parse_ctx_t *ctx = priv_data;

    // Parse until the last newline //
    char *end = memrchr(data, '\n', len);
    if(end == NULL) {
        log_error("invalid csv data, no newline");
        ctx->res = CSV_PARSE_ERR_INVALID;
        return -1;
    }
    len = end - data + 1;
    *end = '\0';

    // Parse lines //
    size_t cols_count;
    char *cols[MAX_CSV_COLS];
    char *p = data;
    while(p < end) {
        p = minicsv_parse_line(p, cols, &cols_count, ARRAY_SIZE(cols));
        if(ctx->head_parsed) {
            ctx->res = ctx->row_cb(ctx, (const char **)cols, cols_count, ctx->priv_data);
        } else {
            ctx->res = parse_head(ctx, (const char **)cols, cols_count);
            ctx->head_parsed = true;
        }
        if(ctx->res != CSV_PARSE_ERR_OK) {
            return -1;
        }
    }

    return len;
}

csv_parse_err_t csv_parse_file(const char *file_path, csv_row_cb_t row_cb, const char *const *names,
                               uint32_t names_count, void *priv_data)
{
    csv_parse_ctx_t ctx;
    ctx.col_names = names;
    memset(ctx.col_idx, UINT32_MAX, names_count * sizeof(uint32_t));
    ctx.col_count = names_count;
    ctx.res = CSV_PARSE_ERR_OK;
    ctx.row_cb = row_cb;
    ctx.priv_data = priv_data;
    ctx.head_parsed = false;

    // Stream file //
    if(file_stream(file_path, stream_cb, &ctx) != FILE_ERR_OK) {
        return ctx.res;
    }
    return CSV_PARSE_ERR_OK;
}

csv_parse_err_t csv_parse(const csv_parse_ctx_t *ctx, const char **cols, const uint32_t cols_count,
                          const csv_item_t *items, uint32_t items_count)
{
    for(uint32_t i = 0; i < items_count; i++) {
        const char *col_name = ctx->col_names[i];
        uint32_t col_idx = ctx->col_idx[i];
        if(col_idx >= cols_count) {
            log_error("col_idx[%s]=%u out of range %u", col_name, col_idx, cols_count);
            return CSV_PARSE_ERR_INVALID;
        }
        const char *col_val = cols[col_idx];
        const csv_item_t *item = &items[i];
        csv_parse_err_t res = item->cb(col_val, item->priv_data);
        if(res != CSV_PARSE_ERR_OK) {
            log_error("parse value '%s' in column '%s' failed", col_name, ctx->col_names[i]);
            return res;
        }
    }
    return CSV_PARSE_ERR_OK;
}

csv_parse_err_t csv_parse_ts(const char *col, void *priv_data)
{
    struct tm tm;
    if(strptime(col, "%Y-%m-%d %H:%M:%S", &tm) == NULL) {
        return CSV_PARSE_ERR_DECODE;
    }
    uint64_t *ts = priv_data;
    *ts = mktime(&tm);
    return CSV_PARSE_ERR_OK;
}

csv_parse_err_t csv_parse_float(const char *col, void *priv_data)
{
    char *end;
    float *val = priv_data;
    *val = strtof(col, &end);
    if(*end != '\0') {
        return CSV_PARSE_ERR_DECODE;
    }
    return CSV_PARSE_ERR_OK;
}

csv_parse_err_t csv_parse_uint8(const char *col, void *priv_data)
{
    char *end;
    uint8_t *val = priv_data;
    *val = strtoul(col, &end, 10);
    if(*end != '\0') {
        return CSV_PARSE_ERR_DECODE;
    }
    return CSV_PARSE_ERR_OK;
}
