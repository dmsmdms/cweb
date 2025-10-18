#include <wasm/calc-crypto.h>

#define RSI_PERIOD 14

float calc_crypto_rsi(const float *prices, uint32_t count, uint32_t offset)
{
    if(count < RSI_PERIOD + 1) {
        return -1.0f;
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
    if(avg_loss == 0.0f) {
        return 100.0f; // Prevent division by zero, RSI is 100
    }
    float rs = avg_gain / avg_loss;
    return 100.0f - (100.0f / (1.0f + rs));
}
