#include <db/db-crypto.h>
#include <db/db-crypto-table.h>
#include <db/db-crypto-calc.h>
#include <calc/calc-crypto-func.h>
#include <core/ai/ai-gboost.h>
#include <core/telebot/telebot-method.h>
#include <core/base/log.h>
#include <core/base/str.h>
#include <inttypes.h>
#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>
#include <ev.h>

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

typedef struct {
    calc_crypto_ctx_t calc;
    ev_timer timer;
    time_t last_ts;
    uint32_t sym_id;
    uint64_t chat_ids[1];
} ai_t;

static ai_t ai = {
    .chat_ids = {
        578815223,
    },
};

static void update_cb(UNUSED struct ev_loop *loop, UNUSED ev_timer *timer, UNUSED int events)
{
    db_cursor_op_t op = DB_CURSOR_OP_SET_RANGE;
    while(true) {
        crypto_t crypto;
        db_err_t res = db_crypto_get_next2(ai.sym_id, ai.last_ts + 1, op, &crypto);
        if(res != DB_ERR_OK) {
            db_txn_abort();
            return;
        }

        calc_crypto_row_t row;
        calc_crypto_main(&ai.calc, &crypto, &row);
        ai_row_t ai_row;
        ai_row.cols[CALC_AI_COL_RSI] = row.rsi;
        ai_row.cols[CALC_AI_COL_TAIL] = row.tail;
        ai_row.cols[CALC_AI_COL_SLOPE] = row.slope;
        ai_row.cols[CALC_AI_COL_WHALES] = crypto.whales;
        ai_row.cols[CALC_AI_COL_LIQUIDITY] = row.liquidity;
        ai_row.cols[CALC_AI_COL_LIQ_BID] = crypto.liq_bid;
        ai_row.cols[CALC_AI_COL_LIQ_ASK] = crypto.liq_ask;
        ai_row.cols[CALC_AI_COL_OB_DELTA] = row.ob_delta;
        ai_row.cols[CALC_AI_COL_BID_ASK_RATIO] = row.bid_ask_ratio;
        ai_row.cols[CALC_AI_COL_VOLUME] = crypto.volume;
        ai_row.cols[CALC_AI_COL_VOLUME_SURGE] = row.volume_surge;
        ai_row.cols[CALC_AI_COL_VOLUME_ACCEL] = row.volume_accel;
        ai_row.cols[CALC_AI_COL_PRICE] = crypto.close;
        ai_row.cols[CALC_AI_COL_HOUR_OF_DAY] = row.hour_of_day;
        ai_row.cols[CALC_AI_COL_MINUTE_OF_DAY] = row.minute_of_day;
        ai_row.cols[CALC_AI_COL_PRICE_CHANGE_3] = row.price_change_3;
        ai_row.cols[CALC_AI_COL_PRICE_CHANGE_10] = row.price_change_10;
        ai_row.cols[CALC_AI_COL_PRICE_VOLATILITY_10] = row.price_volatility_10;
        ai_row.cols[CALC_AI_COL_PRICE_SLOPE_15_PCT] = row.price_slope_15_pct;
        ai_row.cols[CALC_AI_COL_RSI_PCT5] = row.rsi_change_5;
        ai_row.cols[CALC_AI_COL_RSI_SLOPE_10] = row.rsi_slope_10;
        ai_row.cols[CALC_AI_COL_VOLUME_CHANGE_5] = row.volume_change_5;
        ai_row.cols[CALC_AI_COL_VOLUME_MA_RATIO] = row.volume_ma_ratio;
        ai_row.cols[CALC_AI_COL_BID_PRESSURE] = row.bid_pressure;
        ai_row.cols[CALC_AI_COL_ASK_PRESSURE] = row.ask_pressure;
        ai_row.cols[CALC_AI_COL_BID_ASK_DIFF_PCT] = row.bid_ask_diff_pct;
        ai_row.cols[CALC_AI_COL_LIQ_BID_GROWTH_15] = row.liq_bid_growth_15;
        float result;
        if(ai_gb_predict(ai_row.cols, CALC_AI_COL_MAX, &result) != AI_GB_ERR_OK) {
            db_txn_abort();
            return;
        }
        log_debug("ts=%" PRIu64 " close=%.2f label=%u pred=%.2f", crypto.ts, crypto.close, row.label, result);
        op = DB_CURSOR_OP_NEXT;
        ai.last_ts = crypto.ts;

        char buf_mem[4096];
        str_buf_t buf;
        str_buf_init(&buf, buf_mem, sizeof(buf_mem));
        buf_printf(&buf, "💰 Цена: %.2f\n", crypto.close);
        buf_printf(&buf, "🎲 RSI: %.2f\n", row.rsi);
        buf_printf(&buf, "💎 Объём: %.2f\n", crypto.volume);
        buf_printf(&buf, "🏔 Slope: %.2f\n", row.slope);
        buf_printf(&buf, "☝️ liq_bid: %.2f\n", crypto.liq_bid);
        buf_printf(&buf, "👇 liq_ask: %.2f\n", crypto.liq_ask);
        buf_printf(&buf, "⚖️ bid/ask ratio: %.2f\n", row.bid_ask_ratio);
        if(crypto.liq_bid > crypto.liq_ask * 1.5) {
            buf_printf(&buf, "📈 Дисбаланс в пользу покупателей");
        } else if(crypto.liq_ask > crypto.liq_bid * 1.5) {
            buf_printf(&buf, "📉 Давление со стороны продавцов");
        } else {
            buf_printf(&buf, "⚖️ Равновесный стакан");
        }
        for(uint32_t i = 0; i < ARRAY_SIZE(ai.chat_ids); i++) {
            telebot_send_message(ai.chat_ids[i], buf.data);
        }
    }
}

db_err_t db_crypto_ai_train_model(const char *path, const char *sym_name)
{
    const char *tok = getenv("TOK");
    if(tok == NULL) {
        log_error("TOK env variable is not set");
        return DB_ERR_FAIL;
    }
    telebot_set_token(tok);

    db_err_t res = db_crypto_init_calc(sym_name, &ai.sym_id, &ai.calc);
    if(res != DB_ERR_OK) {
        return res;
    }

    ai_row_t *rows = NULL;
    float *labels = NULL;
    if(access(path, F_OK) < 0) {
        rows = malloc((sizeof(ai_row_t) + sizeof(float)) * ROWS_COUNT);
        if(rows == NULL) {
            log_error("malloc failed");
            return DB_ERR_NO_MEM;
        }
        labels = (float *)(rows + ROWS_COUNT);
    }

    uint32_t line_idx = 0;
    crypto_t crypto = { 0 };
    while(true) {
        res = db_crypto_get_next1(ai.sym_id, DB_CURSOR_OP_NEXT, &crypto);
        if(res != DB_ERR_OK) {
            if(res == DB_ERR_NOT_FOUND) {
                break;
            }
            db_txn_abort();
            free(rows);
            return res;
        }

        calc_crypto_row_t row;
        calc_crypto(&ai.calc, &crypto, &row);
        if(rows) {
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
        }
        line_idx++;

        if(line_idx % 10000 == 0) {
            log_debug("exported %u rows", line_idx);
        }
    }
    ai.last_ts = crypto.ts;
    db_txn_abort();

    // Print statistic //
    log_info("read %u rows for '%s'", line_idx, sym_name);
    calc_crypto_log_stat(&ai.calc.stat);

    // Train model //
    if(ai_gb_train_model((float *)rows, labels, line_idx, CALC_AI_COL_MAX, path) != AI_GB_ERR_OK) {
        res = DB_ERR_FAIL;
    }
    free(rows);

    for(uint32_t i = ai.calc.hist.forward.cnt - FCHANGE_PERIOD; i < ai.calc.hist.forward.cnt; i++) {
        const crypto_t *cur = buf_circ_get(&ai.calc.hist.forward, i);
        calc_crypto_row_t row;
        calc_crypto_main(&ai.calc, cur, &row);
    }

    ev_timer_init(&ai.timer, update_cb, 1.0, 1.0);
    ev_timer_start(EV_DEFAULT, &ai.timer);

    return DB_ERR_OK;
}

void db_crypto_ai_deinit(void)
{
    ev_timer_stop(EV_DEFAULT, &ai.timer);
    ai.last_ts = 0;
}
