#include <calc/calc-crypto-func.h>

#define RSI_PERIOD          14
#define TAIL_PERIOD         5
#define SLOPE_PERIOD        10
#define VOLUME_MA_PERIOD    10
#define VOLUME_SURGE_PERIOD 5

calc_crypto_fchange_t calc_crypto_fchange(const calc_crypto_ctx_t *ctx)
{
    calc_crypto_fchange_t res = { 0 };
    const buf_circ_t *win = &ctx->forward;
    if(win->cnt < 1) {
        return res;
    }

    const crypto_t *crypto = buf_circ_get(win, 0);
    float min_price = crypto->close;
    float max_price = crypto->close;
    for(uint32_t i = 1; i < win->cnt; i++) {
        const crypto_t *c = buf_circ_get(win, i);
        if(c->close < min_price) {
            min_price = c->close;
        }
        if(c->close > max_price) {
            max_price = c->close;
        }
    }

    res.growth = (max_price - crypto->close) / crypto->close;
    res.rollback = (crypto->close - min_price) / crypto->close;
    return res;
}

float calc_crypto_fpchange(const calc_crypto_ctx_t *ctx, float cur_price, uint32_t period)
{
    const buf_circ_t *win = &ctx->forward;
    if(win->cnt < period + 1) {
        return 0.0f;
    }

    const crypto_t *next = buf_circ_get(win, period);
    return 100.0f * (next->close - cur_price) / cur_price;
}

float calc_crypto_pct(const buf_circ_t *win, uint32_t period)
{
    if(win->cnt < period) {
        return 0.0f;
    }

    float old = buf_circ_get_float(win, win->cnt - period);
    if(old == 0.0f) {
        return 0.0f;
    }
    float cur = buf_circ_get_float(win, win->cnt - 1);
    return 100.0f * (cur - old) / old;
}

float calc_crypto_rsi(const calc_crypto_ctx_t *ctx)
{
    const buf_circ_t *hist = &ctx->price_hist;
    if(hist->cnt < RSI_PERIOD + 1) {
        return 50.0f;
    }

    float gain_sum = 0.0f;
    float loss_sum = 0.0f;
    for(uint32_t i = hist->cnt - RSI_PERIOD; i < hist->cnt; i++) {
        float cur = buf_circ_get_float(hist, i);
        float prev = buf_circ_get_float(hist, i - 1);
        float delta = cur - prev;
        if(delta > 0.0f) {
            gain_sum += delta;
        } else {
            loss_sum -= delta;
        }
    }

    float avg_gain = gain_sum / RSI_PERIOD;
    float avg_loss = loss_sum / RSI_PERIOD;
    if(avg_loss == 0.0f) {
        return 100.0f;
    }
    float rs = avg_gain / avg_loss;
    return 100.0f - (100.0f / (1.0f + rs));
}

/*
float calc_crypto_tail(const calc_crypto_ctx_t *ctx)
{
    const float *prices = ctx->price_hist;
    uint32_t off = ctx->price_hist_off;
    uint32_t cnt = ctx->price_hist_cnt;
    if(cnt < TAIL_PERIOD) {
        return 0.0f;
    }

    float local_max = prices[off];
    float local_min = prices[off];
    for(uint32_t max_i = off + cnt, i = max_i - TAIL_PERIOD + 1; i < max_i; i++) {
        float price = prices[i % cnt];
        if(price > local_max) {
            local_max = price;
        }
        if(price < local_min) {
            local_min = price;
        }
    }

    float last_close = prices[off];
    float upper_tail = fabsf(local_max - last_close) / last_close;
    float lower_tail = fabsf(last_close - local_min) / last_close;
    return (upper_tail > lower_tail) ? upper_tail : lower_tail;
}
*/

float calc_crypto_slope(const calc_crypto_ctx_t *ctx)
{
    const buf_circ_t *hist = &ctx->price_hist;
    if(hist->cnt < SLOPE_PERIOD) {
        return 0.0f;
    }

    float sum_x = 0.0f;
    float sum_y = 0.0f;
    float sum_xy = 0.0f;
    float sum_x2 = 0.0f;
    uint32_t off = hist->cnt - SLOPE_PERIOD;
    for(uint32_t i = 0; i < SLOPE_PERIOD; i++) {
        float x = i;
        float y = buf_circ_get_float(hist, off + i);
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }

    float denominator = SLOPE_PERIOD * sum_x2 - sum_x * sum_x;
    float slope = (SLOPE_PERIOD * sum_xy - sum_x * sum_y) / denominator;
    float mean_y = sum_y / SLOPE_PERIOD;
    return slope / mean_y;
}

float clacl_crypto_volume_ma_ratio(const calc_crypto_ctx_t *ctx)
{
    const buf_circ_t *hist = &ctx->volume_hist;
    if(hist->cnt < VOLUME_MA_PERIOD) {
        return 0.0f;
    }

    float sum = 0.0f;
    for(uint32_t i = hist->cnt - VOLUME_MA_PERIOD; i < hist->cnt; i++) {
        sum += buf_circ_get_float(hist, i);
    }
    float ma = sum / VOLUME_MA_PERIOD;
    if(ma == 0.0f) {
        return 0.0f;
    }
    float cur = buf_circ_get_float(hist, hist->cnt - 1);
    return cur / ma;
}

/*
float calc_crypto_volume_surge(const calc_crypto_ctx_t *ctx, float cur_volume)
{
    const float *volumes = ctx->volume_hist;
    uint32_t off = ctx->volume_hist_off;
    uint32_t cnt = ctx->volume_hist_cnt;
    if(cnt < VOLUME_SURGE_PERIOD) {
        return 0.0f;
    }

    float sum = 0.0f;
    for(uint32_t max_i = off + cnt, i = max_i - VOLUME_SURGE_PERIOD; i < max_i; i++) {
        sum += volumes[i % cnt];
    }

    float avg = sum / VOLUME_SURGE_PERIOD;
    return (cur_volume - avg) / avg;
}


float calc_crypto_price_volatility(const calc_crypto_ctx_t *ctx, uint32_t period)
{
    const float *prices = ctx->price_hist;
    uint32_t off = ctx->price_hist_off;
    uint32_t cnt = ctx->price_hist_cnt;
    if(cnt < period) {
        return 0.0f;
    }

    float sum = 0.0f;
    for(uint32_t max_i = off + cnt, i = max_i - period; i < max_i; i++) {
        sum += prices[i % cnt];
    }

    float var = 0.0f;
    float mean = sum / period;
    for(uint32_t max_i = off + cnt, i = max_i - period; i < max_i; i++) {
        float diff = prices[i % cnt] - mean;
        var += diff * diff;
    }
    var /= period;
    return 100.0f * sqrtf(var) / (mean + 1e-9);
}
*/
