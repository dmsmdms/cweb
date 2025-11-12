#include <db/db-crypto.h>
#include <db/db-crypto-table.h>
#include <db/db-crypto-calc.h>
#include <calc/calc-crypto-func.h>
#include <core/csv/csv-gen.h>
#include <core/base/log.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

typedef enum {
    CALC_CSV_COL_TS,
    CALC_CSV_COL_RSI,
    CALC_CSV_COL_TAIL,
    CALC_CSV_COL_SLOPE,
    CALC_CSV_COL_WHALES,
    CALC_CSV_COL_LIQUIDITY,
    CALC_CSV_COL_LIQ_BID,
    CALC_CSV_COL_LIQ_ASK,
    CALC_CSV_COL_OB_DELTA,
    CALC_CSV_COL_BID_ASK_RATIO,
    CALC_CSV_COL_VOLUME,
    CALC_CSV_COL_VOLUME_SURGE,
    CALC_CSV_COL_VOLUME_ACCEL,
    CALC_CSV_COL_PRICE,
    CALC_CSV_COL_HOUR_OF_DAY,
    CALC_CSV_COL_MINUTE_OF_DAY,
    CALC_CSV_COL_PRICE_CHANGE_3,
    CALC_CSV_COL_PRICE_CHANGE_10,
    CALC_CSV_COL_PRICE_VOLATILITY_10,
    CALC_CSV_COL_PRICE_SLOPE_15_PCT,
    CALC_CSV_COL_RSI_PCT5,
    CALC_CSV_COL_RSI_SLOPE_10,
    CALC_CSV_COL_VOLUME_CHANGE_5,
    CALC_CSV_COL_VOLUME_MA_RATIO,
    CALC_CSV_COL_BID_PRESSURE,
    CALC_CSV_COL_ASK_PRESSURE,
    CALC_CSV_COL_BID_ASK_DIFF_PCT,
    CALC_CSV_COL_LIQ_BID_GROWTH_15,
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
    uint32_t line_count;
} calc_csv_calc_t;

static const char *const csv_col_names[] = {
    [CALC_CSV_COL_TS] = "timestamp",
    [CALC_CSV_COL_RSI] = "rsi",
    [CALC_CSV_COL_TAIL] = "tail",
    [CALC_CSV_COL_SLOPE] = "slope",
    [CALC_CSV_COL_WHALES] = "whales",
    [CALC_CSV_COL_LIQUIDITY] = "liquidity",
    [CALC_CSV_COL_LIQ_BID] = "liq_bid",
    [CALC_CSV_COL_LIQ_ASK] = "liq_ask",
    [CALC_CSV_COL_OB_DELTA] = "ob_delta",
    [CALC_CSV_COL_BID_ASK_RATIO] = "bid_ask_ratio",
    [CALC_CSV_COL_VOLUME] = "volume",
    [CALC_CSV_COL_VOLUME_SURGE] = "volume_surge",
    [CALC_CSV_COL_VOLUME_ACCEL] = "volume_accel",
    [CALC_CSV_COL_PRICE] = "price",
    [CALC_CSV_COL_HOUR_OF_DAY] = "hour_of_day",
    [CALC_CSV_COL_MINUTE_OF_DAY] = "minute_of_day",
    [CALC_CSV_COL_PRICE_CHANGE_3] = "price_change_3",
    [CALC_CSV_COL_PRICE_CHANGE_10] = "price_change_10",
    [CALC_CSV_COL_PRICE_VOLATILITY_10] = "price_volatility_10",
    [CALC_CSV_COL_PRICE_SLOPE_15_PCT] = "price_slope_15_pct",
    [CALC_CSV_COL_RSI_PCT5] = "rsi_pct5",
    [CALC_CSV_COL_RSI_SLOPE_10] = "rsi_slope_10",
    [CALC_CSV_COL_VOLUME_CHANGE_5] = "volume_change_5",
    [CALC_CSV_COL_VOLUME_MA_RATIO] = "volume_ma_ratio",
    [CALC_CSV_COL_BID_PRESSURE] = "bid_pressure",
    [CALC_CSV_COL_ASK_PRESSURE] = "ask_pressure",
    [CALC_CSV_COL_BID_ASK_DIFF_PCT] = "bid_ask_diff_pct",
    [CALC_CSV_COL_LIQ_BID_GROWTH_15] = "liq_bid_growth_15",
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

    calc_crypto_row_t row;
    calc_crypto(&ctx->calc, &crypto, &row);
    csv_gen_item_t items[] = {
        [CALC_CSV_COL_TS] = CSV_GEN_TS(crypto.ts),
        [CALC_CSV_COL_RSI] = CSV_GEN_FLOAT(row.rsi),
        [CALC_CSV_COL_TAIL] = CSV_GEN_FLOAT(row.tail),
        [CALC_CSV_COL_SLOPE] = CSV_GEN_FLOAT(row.slope),
        [CALC_CSV_COL_WHALES] = CSV_GEN_UINT8(crypto.whales),
        [CALC_CSV_COL_LIQUIDITY] = CSV_GEN_FLOAT(row.liquidity),
        [CALC_CSV_COL_LIQ_BID] = CSV_GEN_FLOAT(crypto.liq_bid),
        [CALC_CSV_COL_LIQ_ASK] = CSV_GEN_FLOAT(crypto.liq_ask),
        [CALC_CSV_COL_OB_DELTA] = CSV_GEN_FLOAT(row.ob_delta),
        [CALC_CSV_COL_BID_ASK_RATIO] = CSV_GEN_FLOAT(row.bid_ask_ratio),
        [CALC_CSV_COL_VOLUME] = CSV_GEN_FLOAT(crypto.volume),
        [CALC_CSV_COL_VOLUME_SURGE] = CSV_GEN_FLOAT(row.volume_surge),
        [CALC_CSV_COL_VOLUME_ACCEL] = CSV_GEN_FLOAT(row.volume_accel),
        [CALC_CSV_COL_PRICE] = CSV_GEN_FLOAT(crypto.close),
        [CALC_CSV_COL_HOUR_OF_DAY] = CSV_GEN_UINT8(row.hour_of_day),
        [CALC_CSV_COL_MINUTE_OF_DAY] = CSV_GEN_UINT8(row.minute_of_day),
        [CALC_CSV_COL_PRICE_CHANGE_3] = CSV_GEN_FLOAT(row.price_change_3),
        [CALC_CSV_COL_PRICE_CHANGE_10] = CSV_GEN_FLOAT(row.price_change_10),
        [CALC_CSV_COL_PRICE_VOLATILITY_10] = CSV_GEN_FLOAT(row.price_volatility_10),
        [CALC_CSV_COL_PRICE_SLOPE_15_PCT] = CSV_GEN_FLOAT(row.price_slope_15_pct),
        [CALC_CSV_COL_RSI_PCT5] = CSV_GEN_FLOAT(row.rsi_change_5),
        [CALC_CSV_COL_RSI_SLOPE_10] = CSV_GEN_FLOAT(row.rsi_slope_10),
        [CALC_CSV_COL_VOLUME_CHANGE_5] = CSV_GEN_FLOAT(row.volume_change_5),
        [CALC_CSV_COL_VOLUME_MA_RATIO] = CSV_GEN_FLOAT(row.volume_ma_ratio),
        [CALC_CSV_COL_BID_PRESSURE] = CSV_GEN_FLOAT(row.bid_pressure),
        [CALC_CSV_COL_ASK_PRESSURE] = CSV_GEN_FLOAT(row.ask_pressure),
        [CALC_CSV_COL_BID_ASK_DIFF_PCT] = CSV_GEN_FLOAT(row.bid_ask_diff_pct),
        [CALC_CSV_COL_LIQ_BID_GROWTH_15] = CSV_GEN_FLOAT(row.liq_bid_growth_15),
        [CALC_CSV_COL_LABEL_1] = CSV_GEN_UINT8(row.label1),
        [CALC_CSV_COL_CHANGE_05] = CSV_GEN_FLOAT(row.change_05),
        [CALC_CSV_COL_CHANGE_15] = CSV_GEN_FLOAT(row.change_15),
        [CALC_CSV_COL_CHANGE_30] = CSV_GEN_FLOAT(row.change_30),
        [CALC_CSV_COL_CHANGE_45] = CSV_GEN_FLOAT(row.change_45),
        [CALC_CSV_COL_LABEL_2] = CSV_GEN_UINT8(row.label2),
        [CALC_CSV_COL_LABEL] = CSV_GEN_UINT8(row.label),
    };
    csv_gen_err_t csv_res = csv_gen(gctx, items, ARRAY_SIZE(items));
    if(csv_res != CSV_GEN_ERR_OK) {
        return csv_res;
    }
    ctx->line_count++;
    if(ctx->line_count % 100000 == 0) {
        log_debug("exported %u rows", ctx->line_count);
    }
    return CSV_GEN_ERR_OK;
}

db_err_t db_crypto_init_calc(const char *sym_name, uint32_t *psym_id, calc_crypto_ctx_t *calc)
{
    db_crypto_sym_t sym;
    db_err_t res = db_crypto_get_sym(sym_name, &sym);
    if(res != DB_ERR_OK) {
        if(res == DB_ERR_NOT_FOUND) {
            log_error("Symbol '%s' not found in DB", sym_name);
        }
        db_txn_abort();
        return res;
    }
    *psym_id = sym.id;

    // Fill forward buffer //
    calc_crypto_init(calc);
    db_cursor_op_t op = DB_CURSOR_OP_SET_RANGE;
    for(uint32_t i = 0; i < FCHANGE_PERIOD; i++) {
        crypto_t crypto;
        res = db_crypto_get_next1(sym.id, op, &crypto);
        if(res != DB_ERR_OK) {
            if(res == DB_ERR_NOT_FOUND) {
                log_error("Not enough data for symbol '%s'", sym_name);
            }
            db_txn_abort();
            return res;
        }
        buf_circ_add(&calc->hist.forward, &crypto);
        op = DB_CURSOR_OP_NEXT;
    }

    return DB_ERR_OK;
}

db_err_t db_crypto_export_calc_csv(const char *csv_path, const char *sym_name)
{
    calc_csv_calc_t ctx = {
        .line_count = 0,
    };
    db_err_t res = db_crypto_init_calc(sym_name, &ctx.sym_id, &ctx.calc);
    if(res != DB_ERR_OK) {
        return res;
    }

    // Generate CSV //
    if(csv_gen_file(csv_path, csv_gen_row, csv_col_names, ARRAY_SIZE(csv_col_names), &ctx) != CSV_GEN_ERR_OK) {
        res = DB_ERR_PARSE;
    }
    db_txn_abort();

    // Print statistic //
    log_info("exported %u rows for '%s'", ctx.line_count, sym_name);
    calc_crypto_log_stat(&ctx.calc.stat);
    return res;
}
