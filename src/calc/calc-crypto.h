#pragma once

#include <core/base/buf.h>
#include <db/db-crypto.h>

#define CALC_CRYPTO_LINES_PER_MINUTE 30
#define CALC_CRYPTO_SIZE_PRICE_HIST  (15 * CALC_CRYPTO_LINES_PER_MINUTE + 1)
#define CALC_CRYPTO_SIZE_VOLUME_HIST (10 * CALC_CRYPTO_LINES_PER_MINUTE + 1)
#define CALC_CRYPTO_SIZE_LIQ_HIST    (15 * CALC_CRYPTO_LINES_PER_MINUTE + 1)
#define CALC_CRYPTO_SIZE_RSI_HIST    (10 * CALC_CRYPTO_LINES_PER_MINUTE + 1)
#define CALC_CRYPTO_SIZE_SLOPE_HIST  (15 * CALC_CRYPTO_LINES_PER_MINUTE + 1)
#define CALC_CRYPTO_SIZE_FORWARD     (180 * CALC_CRYPTO_LINES_PER_MINUTE + 1)

typedef struct {
    float rsi;
    float tail;
    float slope;
    float liquidity;
    float ob_delta;
    float bid_ask_ratio;
    float volume;
    float volume_surge;
    float volume_accel;
    uint32_t hour_of_day;
    uint32_t minute_of_day;
    float price_change_3;
    float price_change_10;
    float price_volatility_10;
    float price_slope_15_pct;
    float rsi_change_5;
    float rsi_slope_10;
    float volume_change_5;
    float volume_ma_ratio;
    float bid_pressure;
    float ask_pressure;
    float bid_ask_diff_pct;
    float liq_bid_growth_15;
    float change_05;
    float change_15;
    float change_30;
    float change_45;
    bool label1;
    bool label2;
    bool label;
} calc_crypto_row_t;

typedef struct {
    uint32_t fchange_cache_cnt;
    uint32_t growth;
    uint32_t rollback;
    uint32_t rsi;
    uint32_t volume;
    uint32_t liq_bid;
    uint32_t rsi_change_5;
    uint32_t price_slope_15_pct;
    uint32_t vol_ma_ratio;
    uint32_t bid_pressure;
    uint32_t liq_bid_growth_15;
    uint32_t derived_signals;
    uint32_t change_05;
    uint32_t change_15;
    uint32_t change_30;
    uint32_t change_45;
    uint32_t label1;
    uint32_t label2;
    uint32_t label;
} calc_crypto_stat_t;

typedef struct {
    crypto_t forward_buf[CALC_CRYPTO_SIZE_FORWARD];
    buf_circ_t forward;

    float price_buf[CALC_CRYPTO_SIZE_PRICE_HIST];
    buf_circ_t price;

    float volume_buf[CALC_CRYPTO_SIZE_VOLUME_HIST];
    buf_circ_t volume;

    float liq_bid_buf[CALC_CRYPTO_SIZE_LIQ_HIST];
    buf_circ_t liq_bid;

    float rsi_buf[CALC_CRYPTO_SIZE_RSI_HIST];
    buf_circ_t rsi;
} calc_crypto_hist_t;

typedef struct {
    const crypto_t *fchange_prev_ptr;
    float fchange_min_price;
    float fchange_max_price;

    const float *rsi_prev_price_ptr;
    float rsi_avg_gain;
    float rsi_avg_loss;

    const float *price_volatility_prev_ptr;
    double price_volatility_sum;
    double price_volatility_sum2;
} calc_crypto_cache_t;

typedef struct {
    calc_crypto_hist_t hist;
    calc_crypto_stat_t stat;
    calc_crypto_cache_t cache;
    float prev_volume_surge;
    bool prev_label1;
    bool prev_label2;
} calc_crypto_ctx_t;

void calc_crypto_init(calc_crypto_ctx_t *ctx);

void calc_crypto(calc_crypto_ctx_t *ctx, const crypto_t *fdata, calc_crypto_row_t *row);

void calc_crypto_log_stat(const calc_crypto_stat_t *stat);
