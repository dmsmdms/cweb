#pragma once

#include <core/base/buf.h>
#include <db/db-crypto.h>

#define CALC_CRYPTO_SIZE_PRICE_HIST  15
#define CALC_CRYPTO_SIZE_VOLUME_HIST 10
#define CALC_CRYPTO_SIZE_LIQ_HIST    15
#define CALC_CRYPTO_SIZE_RSI_HIST    5
#define CALC_CRYPTO_SIZE_SLOPE_HIST  15
#define CALC_CRYPTO_SIZE_FORWARD     180

typedef struct {
    uint64_t ts;
    float close;
    float volume;
    float liq_ask;
    float liq_bid;
    uint32_t whales;
    float rsi;
    float slope;
    float liquidity;
    float rsi_pct5;
    float slope_pct15;
    float volume_ma_ratio;
    float bid_pressure;
    float liq_bid_pct15;
    float change_05;
    float change_15;
    float change_30;
    float change_45;
    bool label1;
    bool label2;
    bool label;
} crypto_calc_row_t;

typedef struct {
    uint32_t growth;
    uint32_t rollback;
    uint32_t rsi;
    uint32_t volume;
    uint32_t liq_bid;
    uint32_t rsi_pct5;
    uint32_t slope_pct15;
    uint32_t vol_ma_ratio;
    uint32_t liq_bid_pressure;
    uint32_t liq_bid_pct15;
    uint32_t derived_signals;
    uint32_t label1;
    uint32_t change_05;
    uint32_t change_15;
    uint32_t change_30;
    uint32_t change_45;
    uint32_t label2;
    uint32_t label;
} crypto_calc_stat_t;

typedef struct {
    crypto_calc_stat_t stat;
    bool prev_label1;
    bool prev_label2;

    crypto_t forward_buf[CALC_CRYPTO_SIZE_FORWARD];
    buf_circ_t forward;

    float price_hist_buf[CALC_CRYPTO_SIZE_PRICE_HIST];
    buf_circ_t price_hist;

    float volume_hist_buf[CALC_CRYPTO_SIZE_VOLUME_HIST];
    buf_circ_t volume_hist;

    float liq_bid_hist_buf[CALC_CRYPTO_SIZE_LIQ_HIST];
    buf_circ_t liq_bid_hist;

    float rsi_hist_buf[CALC_CRYPTO_SIZE_RSI_HIST];
    buf_circ_t rsi_hist;

    float slope_hist_buf[CALC_CRYPTO_SIZE_SLOPE_HIST];
    buf_circ_t slope_hist;
} calc_crypto_ctx_t;

void calc_crypto_init(calc_crypto_ctx_t *ctx);

void calc_crypto(calc_crypto_ctx_t *ctx, const crypto_t *fdata, crypto_calc_row_t *row);

void calc_crypto_log_stat(const crypto_calc_stat_t *stat);
