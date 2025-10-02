#include <calc/calc-crypto-func.h>
#include <calc/calc-math.h>
#include <core/base/log.h>
#include <math.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

static void calc_crypto_fchange_min_max(calc_crypto_ctx_t *ctx)
{
    const buf_circ_t *win = &ctx->hist.forward;
    const crypto_t *crypto = buf_circ_get(win, win->cnt - FCHANGE_PERIOD);
    float min_price = crypto->close;
    float max_price = crypto->close;

    for(uint32_t i = win->cnt - FCHANGE_PERIOD + 1; i < win->cnt; i++) {
        crypto = buf_circ_get(win, i);
        if(crypto->close < min_price) {
            min_price = crypto->close;
        }
        if(crypto->close > max_price) {
            max_price = crypto->close;
        }
    }

    calc_crypto_cache_t *cache = &ctx->cache;
    cache->fchange_min_price = min_price;
    cache->fchange_max_price = max_price;
}

calc_crypto_fchange_t calc_crypto_fchange(calc_crypto_ctx_t *ctx)
{
    calc_crypto_fchange_t res = { 0 };
    const buf_circ_t *win = &ctx->hist.forward;
    if(win->cnt < FCHANGE_PERIOD) {
        return res;
    }

    calc_crypto_cache_t *cache = &ctx->cache;
    const crypto_t *cur = buf_circ_get(win, win->cnt - 1);
    const crypto_t *prev = buf_circ_get(win, win->cnt - 2);

    if(cache->fchange_prev_ptr == prev) {
        const crypto_t *old = buf_circ_get(win, win->cnt - FCHANGE_PERIOD - 1);
        if((old->close == cache->fchange_min_price) || (old->close == cache->fchange_max_price)) {
            calc_crypto_fchange_min_max(ctx);
        } else {
            if(cur->close < cache->fchange_min_price) {
                cache->fchange_min_price = cur->close;
            }
            if(cur->close > cache->fchange_max_price) {
                cache->fchange_max_price = cur->close;
            }
            ctx->stat.fchange_cache_cnt++;
        }
    } else {
        if(cache->fchange_prev_ptr) {
            log_warn("cache invalidated");
        }
        calc_crypto_fchange_min_max(ctx);
    }

    cache->fchange_prev_ptr = cur;
    res.growth = (cache->fchange_max_price - cur->close) / cur->close;
    res.rollback = (cur->close - cache->fchange_min_price) / cur->close;

    return res;
}

float calc_crypto_fpchange(const calc_crypto_ctx_t *ctx, float cur_price, uint32_t period)
{
    const buf_circ_t *win = &ctx->hist.forward;
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

    uint32_t last_idx = win->cnt - 1;
    float old = buf_circ_get_float(win, last_idx - period);
    float cur = buf_circ_get_float(win, last_idx);
    float pct = 100.0f * (cur - old) / old;
    return isfinite(pct) ? pct : 0.0f;
}

float calc_crypto_rsi(calc_crypto_ctx_t *ctx)
{
    const buf_circ_t *hist = &ctx->hist.price;
    if(hist->cnt < RSI_PERIOD + 1) {
        return 50.0f;
    }

    calc_crypto_cache_t *cache = &ctx->cache;
    const float *cur_ptr = buf_circ_get(hist, hist->cnt - 1);
    const float *prev_ptr = buf_circ_get(hist, hist->cnt - 2);
    float gain_sum = 0.0f;
    float loss_sum = 0.0f;

    if(cache->rsi_prev_price_ptr == prev_ptr) {
        gain_sum = cache->rsi_avg_gain * RSI_PERIOD;
        loss_sum = cache->rsi_avg_loss * RSI_PERIOD;
        float delta = *cur_ptr - *prev_ptr;
        if(delta > 0.0f) {
            gain_sum += delta;
        } else {
            loss_sum -= delta;
        }

        float old_cur = buf_circ_get_float(hist, hist->cnt - RSI_PERIOD - 1);
        float old_prev = buf_circ_get_float(hist, hist->cnt - RSI_PERIOD - 2);
        float old_delta = old_cur - old_prev;
        if(old_delta > 0.0f) {
            gain_sum -= old_delta;
        } else {
            loss_sum += old_delta;
        }
    } else {
        if(cache->rsi_prev_price_ptr) {
            log_warn("cache invalidated");
        }
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
    }

    float avg_gain = gain_sum / RSI_PERIOD;
    float avg_loss = loss_sum / RSI_PERIOD;
    cache->rsi_prev_price_ptr = cur_ptr;
    cache->rsi_avg_gain = avg_gain;
    cache->rsi_avg_loss = avg_loss;

    if(avg_loss == 0.0f) {
        return 100.0f;
    }
    float rs = avg_gain / avg_loss;
    return 100.0f - (100.0f / (1.0f + rs));
}

float calc_crypto_tail(const calc_crypto_ctx_t *ctx)
{
    const buf_circ_t *hist = &ctx->hist.price;
    if(hist->cnt < TAIL_PERIOD) {
        return 0.0f;
    }

    uint32_t off = hist->cnt - TAIL_PERIOD;
    float local_max = buf_circ_get_float(hist, off);
    float local_min = local_max;
    for(uint32_t i = off + 1; i < hist->cnt; i++) {
        float price = buf_circ_get_float(hist, i);
        if(price > local_max) {
            local_max = price;
        }
        if(price < local_min) {
            local_min = price;
        }
    }

    float last_close = buf_circ_get_float(hist, hist->cnt - 1);
    float upper_tail = fabsf(local_max - last_close) / last_close;
    float lower_tail = fabsf(last_close - local_min) / last_close;
    return (upper_tail > lower_tail) ? upper_tail : lower_tail;
}

float calc_crypto_slope(const calc_crypto_ctx_t *ctx)
{
    const buf_circ_t *hist = &ctx->hist.price;
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

float clac_crypto_volume_ma_ratio(const calc_crypto_ctx_t *ctx)
{
    const buf_circ_t *hist = &ctx->hist.volume;
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

float calc_crypto_volume_surge(const calc_crypto_ctx_t *ctx)
{
    const buf_circ_t *hist = &ctx->hist.volume;
    if(hist->cnt < VOLUME_SURGE_PERIOD) {
        return 0.0f;
    }

    float sum = 0.0f;
    for(uint32_t i = hist->cnt - VOLUME_SURGE_PERIOD; i < hist->cnt; i++) {
        sum += buf_circ_get_float(hist, i);
    }

    float cur = buf_circ_get_float(hist, hist->cnt - 1);
    float avg = sum / VOLUME_SURGE_PERIOD;
    return (cur - avg) / avg;
}

float calc_crypto_price_volatility(calc_crypto_ctx_t *ctx, uint32_t period)
{
    const buf_circ_t *hist = &ctx->hist.price;
    if(hist->cnt < period) {
        return 0.0f;
    }

    calc_crypto_cache_t *cache = &ctx->cache;
    const float *cur_ptr = buf_circ_get(hist, hist->cnt - 1);
    const float *prev_ptr = buf_circ_get(hist, hist->cnt - 2);
    double sum = 0.0f;
    double sum2 = 0.0f;

    if(cache->price_volatility_prev_ptr == prev_ptr) {
        float cur = *cur_ptr;
        sum = cache->price_volatility_sum + cur;
        sum2 = cache->price_volatility_sum2 + cur * cur;

        float old = buf_circ_get_float(hist, hist->cnt - period - 1);
        sum -= old;
        sum2 -= old * old;
    } else {
        if(cache->price_volatility_prev_ptr) {
            log_warn("cache invalidated");
        }
        for(uint32_t i = hist->cnt - period; i < hist->cnt; i++) {
            float v = buf_circ_get_float(hist, i);
            sum += v;
            sum2 += v * v;
        }
    }
    cache->price_volatility_prev_ptr = cur_ptr;
    cache->price_volatility_sum = sum;
    cache->price_volatility_sum2 = sum2;

    double mean = sum / period;
    double var = (sum2 / period) - mean * mean;
    return 100.0 * calc_sqrt(var) / mean;
}
