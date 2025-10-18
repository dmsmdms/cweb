#include <wasm/calc-crypto.h>

#define NAN          __builtin_nanf("")
#define RSI_PERIOD   14
#define TAIL_PERIOD  5
#define SLOPE_PERIOD 10

static float fabs(float x)
{
    return __builtin_fabsf(x);
}

float calc_crypto_rsi(const float *prices, uint32_t count, uint32_t offset)
{
    if(count < RSI_PERIOD + 1) {
        return NAN;
    }
    float gain_sum = 0.0f;
    float loss_sum = 0.0f;
    for(uint32_t i = offset + count - RSI_PERIOD, max_i = offset + count; i < max_i; i++) {
        float delta = prices[i % count] - prices[(i - 1) % count];
        if(delta > 0.0f) {
            gain_sum += delta;
        } else {
            loss_sum -= delta;
        }
    }

    float avg_gain = gain_sum / RSI_PERIOD;
    float avg_loss = loss_sum / RSI_PERIOD;
    float rs = avg_gain / avg_loss;
    return 100.0f - (100.0f / (1.0f + rs));
}

float calc_crypto_tail(const float *prices, uint32_t count, uint32_t offset)
{
    if(count < TAIL_PERIOD) {
        return NAN;
    }
    float local_max = prices[offset];
    float local_min = prices[offset];
    for(uint32_t i = offset + count - TAIL_PERIOD, max_i = offset + count; i < max_i; i++) {
        float price = prices[i % count];
        if(price > local_max) {
            local_max = price;
        }
        if(price < local_min) {
            local_min = price;
        }
    }

    float last_close = prices[offset];
    float upper_tail = fabs(local_max - last_close) / last_close;
    float lower_tail = fabs(last_close - local_min) / last_close;
    return (upper_tail > lower_tail) ? upper_tail : lower_tail;
}

float calc_crypto_slope(const float *prices, uint32_t count, uint32_t offset)
{
    if(count < SLOPE_PERIOD) {
        return NAN;
    }
    float sum_x = 0.0f;
    float sum_y = 0.0f;
    float sum_xy = 0.0f;
    float sum_x2 = 0.0f;
    offset += count - SLOPE_PERIOD;
    for(uint32_t i = 0; i < SLOPE_PERIOD; i++) {
        float x = i;
        float y = prices[(offset + i) % count];
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }

    float denominator = SLOPE_PERIOD * sum_x2 - sum_x * sum_x;
    float slope = (SLOPE_PERIOD * sum_xy - sum_x * sum_y) / denominator;
    float mean_y = sum_y / SLOPE_PERIOD;
    return slope / mean_y; // Normalized slope
}
