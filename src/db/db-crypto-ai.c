#include <db/db-crypto.h>
#include <db/db-crypto-table.h>
#include <db/db-crypto-calc.h>
#include <calc/calc-crypto-func.h>
#include <core/ai/ai-gboost.h>
#include <core/base/log.h>
#include <malloc.h>

#define ROWS_COUNT (20 * 1000 * 1000)

LOG_MOD_INIT(LOG_LVL_DEFAULT)

typedef enum {
    CALC_AI_COL_RSI,
    CALC_AI_COL_TAIL,
    CALC_AI_COL_SLOPE,
    CALC_AI_COL_WHALES,
    CALC_AI_COL_LIQUIDITY,
    CALC_AI_COL_LIQ_BID,
    CALC_AI_COL_LIQ_ASK,
    CALC_AI_COL_OB_DELTA,
    CALC_AI_COL_BID_ASK_RATIO,
    CALC_AI_COL_VOLUME,
    CALC_AI_COL_VOLUME_SURGE,
    CALC_AI_COL_VOLUME_ACCEL,
    CALC_AI_COL_PRICE,
    CALC_AI_COL_HOUR_OF_DAY,
    CALC_AI_COL_MINUTE_OF_DAY,
    CALC_AI_COL_PRICE_CHANGE_3,
    CALC_AI_COL_PRICE_CHANGE_10,
    CALC_AI_COL_PRICE_VOLATILITY_10,
    CALC_AI_COL_PRICE_SLOPE_15_PCT,
    CALC_AI_COL_RSI_PCT5,
    CALC_AI_COL_RSI_SLOPE_10,
    CALC_AI_COL_VOLUME_CHANGE_5,
    CALC_AI_COL_VOLUME_MA_RATIO,
    CALC_AI_COL_BID_PRESSURE,
    CALC_AI_COL_ASK_PRESSURE,
    CALC_AI_COL_BID_ASK_DIFF_PCT,
    CALC_AI_COL_LIQ_BID_GROWTH_15,
    CALC_AI_COL_MAX,
} ai_col_t;

typedef struct {
    float cols[CALC_AI_COL_MAX];
} ai_row_t;

db_err_t db_crypto_ai_train_model(const char *path, const char *sym_name)
{
    calc_crypto_ctx_t calc;
    uint32_t sym_id;
    db_err_t res = db_crypto_init_calc(sym_name, &sym_id, &calc);
    if(res != DB_ERR_OK) {
        return res;
    }

    ai_row_t *rows = malloc((sizeof(ai_row_t) + sizeof(float)) * ROWS_COUNT);
    if(rows == NULL) {
        log_error("malloc failed");
        return DB_ERR_NO_MEM;
    }
    float *labels = (float *)(rows + ROWS_COUNT);

    uint32_t line_idx = 0;
    while(true) {
        crypto_t crypto;
        res = db_crypto_get_next1(sym_id, DB_CURSOR_OP_NEXT, &crypto);
        if(res != DB_ERR_OK) {
            if(res == DB_ERR_NOT_FOUND) {
                break;
            }
            db_txn_abort();
            free(rows);
            return res;
        }

        calc_crypto_row_t row;
        calc_crypto(&calc, &crypto, &row);
        ai_row_t *ai_row = &rows[line_idx];
        ai_row->cols[CALC_AI_COL_RSI] = row.rsi;
        ai_row->cols[CALC_AI_COL_TAIL] = row.tail;
        ai_row->cols[CALC_AI_COL_SLOPE] = row.slope;
        ai_row->cols[CALC_AI_COL_WHALES] = crypto.whales;
        ai_row->cols[CALC_AI_COL_LIQUIDITY] = row.liquidity;
        ai_row->cols[CALC_AI_COL_LIQ_BID] = crypto.liq_bid;
        ai_row->cols[CALC_AI_COL_LIQ_ASK] = crypto.liq_ask;
        ai_row->cols[CALC_AI_COL_OB_DELTA] = row.ob_delta;
        ai_row->cols[CALC_AI_COL_BID_ASK_RATIO] = row.bid_ask_ratio;
        ai_row->cols[CALC_AI_COL_VOLUME] = crypto.volume;
        ai_row->cols[CALC_AI_COL_VOLUME_SURGE] = row.volume_surge;
        ai_row->cols[CALC_AI_COL_VOLUME_ACCEL] = row.volume_accel;
        ai_row->cols[CALC_AI_COL_PRICE] = crypto.close;
        ai_row->cols[CALC_AI_COL_HOUR_OF_DAY] = row.hour_of_day;
        ai_row->cols[CALC_AI_COL_MINUTE_OF_DAY] = row.minute_of_day;
        ai_row->cols[CALC_AI_COL_PRICE_CHANGE_3] = row.price_change_3;
        ai_row->cols[CALC_AI_COL_PRICE_CHANGE_10] = row.price_change_10;
        ai_row->cols[CALC_AI_COL_PRICE_VOLATILITY_10] = row.price_volatility_10;
        ai_row->cols[CALC_AI_COL_PRICE_SLOPE_15_PCT] = row.price_slope_15_pct;
        ai_row->cols[CALC_AI_COL_RSI_PCT5] = row.rsi_change_5;
        ai_row->cols[CALC_AI_COL_RSI_SLOPE_10] = row.rsi_slope_10;
        ai_row->cols[CALC_AI_COL_VOLUME_CHANGE_5] = row.volume_change_5;
        ai_row->cols[CALC_AI_COL_VOLUME_MA_RATIO] = row.volume_ma_ratio;
        ai_row->cols[CALC_AI_COL_BID_PRESSURE] = row.bid_pressure;
        ai_row->cols[CALC_AI_COL_ASK_PRESSURE] = row.ask_pressure;
        ai_row->cols[CALC_AI_COL_BID_ASK_DIFF_PCT] = row.bid_ask_diff_pct;
        ai_row->cols[CALC_AI_COL_LIQ_BID_GROWTH_15] = row.liq_bid_growth_15;
        labels[line_idx] = row.label;
        line_idx++;
    }
    db_txn_abort();

    // Print statistic //
    log_info("read %u rows for '%s'", line_idx, sym_name);
    calc_crypto_log_stat(&calc.stat);

    // Train model //
    if(ai_gb_train_model(rows->cols, labels, line_idx, CALC_AI_COL_MAX, path) != AI_GB_ERR_OK) {
        res = DB_ERR_FAIL;
    }
    free(rows);
    return res;
}
