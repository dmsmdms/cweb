#include <core/csv/csv-gen.h>
#include <core/base/log.h>
#include <inttypes.h>
#include <stdio.h>
#include <time.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

typedef struct csv_gen_ctx {
    const char *const *names; ///< Column names
    FILE *file;               ///< File pointer for CSV output
} csv_gen_ctx_t;

csv_gen_err_t csv_gen_file(const char *file_path, csv_row_gen_cb_t row_cb, const char *const *names,
                           uint32_t names_count, void *priv_data)
{
    FILE *file = fopen(file_path, "w");
    if(file == NULL) {
        log_error("fopen(%s) failed", file_path);
        return CSV_GEN_ERR_FILE;
    }
    for(uint32_t i = 0; i < names_count - 1; i++) {
        fprintf(file, "%s,", names[i]);
    }
    fprintf(file, "%s\n", names[names_count - 1]);

    // Create CSV generation context //
    csv_gen_ctx_t ctx = {
        .names = names,
        .file = file,
    };
    csv_gen_err_t res = CSV_GEN_ERR_OK;
    while(res == CSV_GEN_ERR_OK) {
        res = row_cb(&ctx, priv_data);
    }
    fclose(file);
    return res == CSV_GEN_ERR_EOF ? CSV_GEN_ERR_OK : res;
}

csv_gen_err_t csv_gen(const csv_gen_ctx_t *ctx, const csv_gen_item_t *items, uint32_t items_count)
{
    for(uint32_t i = 0; i < items_count; i++) {
        const csv_gen_item_t *item = &items[i];
        csv_gen_err_t res = item->cb(ctx, item->val);
        if(res != CSV_GEN_ERR_OK) {
            log_error("gen item '%s' failed", ctx->names[i]);
            return res;
        }
        if(i < items_count - 1) {
            fputc(',', ctx->file);
        }
    }
    fputc('\n', ctx->file);
    return CSV_GEN_ERR_OK;
}

csv_gen_err_t csv_gen_ts(const csv_gen_ctx_t *ctx, csv_gen_val_t val)
{
    struct tm tm;
    localtime_r((time_t *)&val.u64val, &tm);
    char buf[64];
    strftime(buf, sizeof(buf) - 1, "%Y-%m-%d %H:%M:%S", &tm);
    fputs(buf, ctx->file);
    return CSV_GEN_ERR_OK;
}

csv_gen_err_t csv_gen_uint8(const csv_gen_ctx_t *ctx, csv_gen_val_t val)
{
    fprintf(ctx->file, "%" PRIu8, val.u8val);
    return CSV_GEN_ERR_OK;
}

csv_gen_err_t csv_gen_float(const csv_gen_ctx_t *ctx, csv_gen_val_t val)
{
    fprintf(ctx->file, "%.6f", val.fval);
    return CSV_GEN_ERR_OK;
}
