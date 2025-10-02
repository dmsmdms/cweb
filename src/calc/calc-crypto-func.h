#pragma once

#include <calc/calc-crypto.h>

typedef struct {
    float growth;
    float rollback;
} calc_crypto_fchange_t;

calc_crypto_fchange_t calc_crypto_fchange(const calc_crypto_ctx_t *ctx);

float calc_crypto_fpchange(const calc_crypto_ctx_t *ctx, float cur_price, uint32_t period);

float calc_crypto_pct(const buf_circ_t *win, uint32_t period);

float calc_crypto_rsi(const calc_crypto_ctx_t *ctx);

// float calc_crypto_tail(const calc_crypto_ctx_t *ctx);

float calc_crypto_slope(const calc_crypto_ctx_t *ctx);

float clacl_crypto_volume_ma_ratio(const calc_crypto_ctx_t *ctx);

// float calc_crypto_volume_surge(const calc_crypto_ctx_t *ctx, float cur_volume);

// float calc_crypto_price_pct(const calc_crypto_ctx_t *ctx, uint32_t period);

// float calc_crypto_price_volatility(const calc_crypto_ctx_t *ctx, uint32_t period);
