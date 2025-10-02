#pragma once

#include <calc/calc-crypto.h>

#define FCHANGE_PERIOD      (180 * CALC_CRYPTO_LINES_PER_MINUTE)
#define RSI_PERIOD          14
#define TAIL_PERIOD         5
#define SLOPE_PERIOD        10
#define VOLUME_SURGE_PERIOD 5
#define VOLUME_MA_PERIOD    (10 * CALC_CRYPTO_LINES_PER_MINUTE)

STATIC_ASSERT(CALC_CRYPTO_SIZE_FORWARD > FCHANGE_PERIOD);
STATIC_ASSERT(CALC_CRYPTO_SIZE_PRICE_HIST > RSI_PERIOD);
STATIC_ASSERT(CALC_CRYPTO_SIZE_PRICE_HIST > TAIL_PERIOD);
STATIC_ASSERT(CALC_CRYPTO_SIZE_PRICE_HIST > SLOPE_PERIOD);
STATIC_ASSERT(CALC_CRYPTO_SIZE_VOLUME_HIST > VOLUME_SURGE_PERIOD);
STATIC_ASSERT(CALC_CRYPTO_SIZE_VOLUME_HIST > VOLUME_MA_PERIOD);

typedef struct {
    float growth;
    float rollback;
} calc_crypto_fchange_t;

calc_crypto_fchange_t calc_crypto_fchange(calc_crypto_ctx_t *ctx);

float calc_crypto_fpchange(const calc_crypto_ctx_t *ctx, float cur_price, uint32_t period);

float calc_crypto_pct(const buf_circ_t *win, uint32_t period);

float calc_crypto_rsi(calc_crypto_ctx_t *ctx);

float calc_crypto_tail(const calc_crypto_ctx_t *ctx);

float calc_crypto_slope(const calc_crypto_ctx_t *ctx);

float clac_crypto_volume_ma_ratio(const calc_crypto_ctx_t *ctx);

float calc_crypto_volume_surge(const calc_crypto_ctx_t *ctx);

float calc_crypto_price_volatility(calc_crypto_ctx_t *ctx, uint32_t period);
