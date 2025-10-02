#include <db/db-crypto.h>
#include <db/db-crypto-table.h>
#include <calc/calc-crypto.h>
#include <core/csv/csv-gen.h>
#include <core/base/log.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

typedef enum {
    CALC_CSV_COL_TS,
    CALC_CSV_COL_RSI,
    CALC_CSV_COL_SLOPE,
    CALC_CSV_COL_WHALES,
    CALC_CSV_COL_LIQ_BID,
    CALC_CSV_COL_LIQ_ASK,
    CALC_CSV_COL_VOLUME,
    CALC_CSV_COL_CLOSE,
    CALC_CSV_COL_RSI_PCT5,
    CALC_CSV_COL_SLOPE_PCT15,
    CALC_CSV_COL_VOLUME_MA_RATIO,
    CALC_CSV_COL_BID_PRESSURE,
    CALC_CSV_COL_LABEL_1,
    CALC_CSV_COL_CHANGE_05,
    CALC_CSV_COL_CHANGE_15,
    CALC_CSV_COL_CHANGE_30,
    CALC_CSV_COL_CHANGE_45,
    CALC_CSV_COL_LABEL_2,
    CALC_CSV_COL_LABEL,
    CALC_CSV_COL_MAX,
} calc_csv_col_t;

typedef struct {
    calc_crypto_ctx_t calc;
    uint32_t sym_id;
} calc_csv_calc_t;

static const char *const csv_col_names[] = {
    [CALC_CSV_COL_TS] = "timestamp",
    [CALC_CSV_COL_RSI] = "rsi",
    [CALC_CSV_COL_SLOPE] = "slope",
    [CALC_CSV_COL_WHALES] = "whales",
    [CALC_CSV_COL_LIQ_BID] = "liq_bid",
    [CALC_CSV_COL_LIQ_ASK] = "liq_ask",
    [CALC_CSV_COL_VOLUME] = "volume",
    [CALC_CSV_COL_CLOSE] = "price",
    [CALC_CSV_COL_RSI_PCT5] = "rsi_pct5",
    [CALC_CSV_COL_SLOPE_PCT15] = "slope_pct15",
    [CALC_CSV_COL_VOLUME_MA_RATIO] = "volume_ma_ratio",
    [CALC_CSV_COL_BID_PRESSURE] = "bid_pressure",
    [CALC_CSV_COL_LABEL_1] = "label_1",
    [CALC_CSV_COL_CHANGE_05] = "change_05",
    [CALC_CSV_COL_CHANGE_15] = "change_15",
    [CALC_CSV_COL_CHANGE_30] = "change_30",
    [CALC_CSV_COL_CHANGE_45] = "change_45",
    [CALC_CSV_COL_LABEL_2] = "label_2",
    [CALC_CSV_COL_LABEL] = "label",
};
STATIC_ASSERT(ARRAY_SIZE(csv_col_names) == CALC_CSV_COL_MAX);

static csv_gen_err_t csv_gen_row(const csv_gen_ctx_t *gctx, void *priv_data)
{
    calc_csv_calc_t *ctx = priv_data;
    crypto_t crypto;
    db_err_t res = db_crypto_get_next1(ctx->sym_id, DB_CURSOR_OP_NEXT, &crypto);
    if(res != DB_ERR_OK) {
        if(res == DB_ERR_NOT_FOUND) {
            return CSV_GEN_ERR_EOF;
        }
        return CSV_GEN_ERR_DATA;
    }

    crypto_calc_row_t row;
    calc_crypto(&ctx->calc, &crypto, &row);
    csv_gen_item_t items[] = {
        [CALC_CSV_COL_TS] = CSV_GEN_TS(row.ts),
        [CALC_CSV_COL_RSI] = CSV_GEN_FLOAT(row.rsi),
        [CALC_CSV_COL_SLOPE] = CSV_GEN_FLOAT(row.slope),
        [CALC_CSV_COL_WHALES] = CSV_GEN_UINT8(row.whales),
        [CALC_CSV_COL_LIQ_BID] = CSV_GEN_FLOAT(row.liq_bid),
        [CALC_CSV_COL_LIQ_ASK] = CSV_GEN_FLOAT(row.liq_ask),
        [CALC_CSV_COL_VOLUME] = CSV_GEN_FLOAT(row.volume),
        [CALC_CSV_COL_CLOSE] = CSV_GEN_FLOAT(row.close),
        [CALC_CSV_COL_RSI_PCT5] = CSV_GEN_FLOAT(row.rsi_pct5),
        [CALC_CSV_COL_SLOPE_PCT15] = CSV_GEN_FLOAT(row.slope_pct15),
        [CALC_CSV_COL_VOLUME_MA_RATIO] = CSV_GEN_FLOAT(row.volume_ma_ratio),
        [CALC_CSV_COL_BID_PRESSURE] = CSV_GEN_FLOAT(row.bid_pressure),
        [CALC_CSV_COL_LABEL_1] = CSV_GEN_UINT8(row.label1),
        [CALC_CSV_COL_CHANGE_05] = CSV_GEN_FLOAT(row.change_05),
        [CALC_CSV_COL_CHANGE_15] = CSV_GEN_FLOAT(row.change_15),
        [CALC_CSV_COL_CHANGE_30] = CSV_GEN_FLOAT(row.change_30),
        [CALC_CSV_COL_CHANGE_45] = CSV_GEN_FLOAT(row.change_45),
        [CALC_CSV_COL_LABEL_2] = CSV_GEN_UINT8(row.label2),
        [CALC_CSV_COL_LABEL] = CSV_GEN_UINT8(row.label),
    };
    return csv_gen(gctx, items, ARRAY_SIZE(items));
}

db_err_t db_crypto_export_calc_csv(const char *csv_path, const char *sym_name)
{
    calc_csv_calc_t ctx;
    db_err_t res = db_crypto_get_sym(sym_name, &ctx.sym_id);
    if(res != DB_ERR_OK) {
        if(res == DB_ERR_NOT_FOUND) {
            log_error("Symbol '%s' not found in DB", sym_name);
        }
        db_txn_abort();
        return res;
    }

    // Fill forward buffer //
    calc_crypto_init(&ctx.calc);
    db_cursor_op_t op = DB_CURSOR_OP_SET_RANGE;
    for(uint32_t i = 0; i < CALC_CRYPTO_SIZE_FORWARD; i++) {
        crypto_t crypto;
        res = db_crypto_get_next1(ctx.sym_id, op, &crypto);
        if(res != DB_ERR_OK) {
            if(res == DB_ERR_NOT_FOUND) {
                log_error("Not enough data for symbol '%s'", sym_name);
            }
            db_txn_abort();
            return res;
        }
        buf_circ_add(&ctx.calc.forward, &crypto);
        op = DB_CURSOR_OP_NEXT;
    }

    // Generate CSV //
    if(csv_gen_file(csv_path, csv_gen_row, csv_col_names, ARRAY_SIZE(csv_col_names), &ctx) != CSV_GEN_ERR_OK) {
        res = DB_ERR_PARSE;
    }
    db_txn_abort();

    // Print statistic //
    calc_crypto_log_stat(&ctx.calc.stat);
    return res;
}
