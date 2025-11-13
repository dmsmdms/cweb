#include <calc/calc-crypto-func.h>
#include <core/base/log.h>
#include <strings.h>
#include <time.h>

LOG_MOD_INIT(LOG_LVL_DEFAULT)

#define GROWTH_MIN   0.003
#define GROWTH_MAX   0.32
#define ROLLBACK_MIN 0

#define RSI_MIN      45
#define RSI_MAX      70
#define VOLUME_MIN   0
#define LIQ_BID_KOEF 1.5

#define RSI_PCT5_MIN         4.5
#define SLOPE_PCT15_MIN      0.18
#define VOL_MA_RATIO_MIN     1.55
#define LIQ_BID_PRESSURE_MIN 0.77
#define LIQ_BID_PCT15_MIN    99

#define CHANGE_05_MIN 0
#define CHANGE_15_MIN 0.15
#define CHANGE_30_MIN 0.25
#define CHANGE_45_MIN 0.35

#define PERIOD_3  (3 * CALC_CRYPTO_LINES_PER_MINUTE)
#define PERIOD_5  (5 * CALC_CRYPTO_LINES_PER_MINUTE)
#define PERIOD_10 (10 * CALC_CRYPTO_LINES_PER_MINUTE)
#define PERIOD_15 (15 * CALC_CRYPTO_LINES_PER_MINUTE)
#define PERIOD_30 (30 * CALC_CRYPTO_LINES_PER_MINUTE)
#define PERIOD_45 (45 * CALC_CRYPTO_LINES_PER_MINUTE)

STATIC_ASSERT(CALC_CRYPTO_SIZE_PRICE_HIST > PERIOD_15);
STATIC_ASSERT(CALC_CRYPTO_SIZE_VOLUME_HIST > PERIOD_5);
STATIC_ASSERT(CALC_CRYPTO_SIZE_RSI_HIST > PERIOD_10);
STATIC_ASSERT(CALC_CRYPTO_SIZE_LIQ_HIST > PERIOD_15);

void calc_crypto_init(calc_crypto_ctx_t *ctx)
{
    // Clean values and statistic //
    bzero(&ctx->stat, sizeof(calc_crypto_stat_t));
    bzero(&ctx->cache, sizeof(calc_crypto_cache_t));
    ctx->prev_volume_surge = 0.0f;
    ctx->prev_label1 = false;
    ctx->prev_label2 = false;

    // Initialize circular buffers //
    buf_circ_init(&ctx->hist.forward, ctx->hist.forward_buf, CALC_CRYPTO_SIZE_FORWARD, sizeof(crypto_t));
    buf_circ_init(&ctx->hist.price, ctx->hist.price_buf, CALC_CRYPTO_SIZE_PRICE_HIST, sizeof(float));
    buf_circ_init(&ctx->hist.volume, ctx->hist.volume_buf, CALC_CRYPTO_SIZE_VOLUME_HIST, sizeof(float));
    buf_circ_init(&ctx->hist.liq_bid, ctx->hist.liq_bid_buf, CALC_CRYPTO_SIZE_LIQ_HIST, sizeof(float));
    buf_circ_init(&ctx->hist.rsi, ctx->hist.rsi_buf, CALC_CRYPTO_SIZE_RSI_HIST, sizeof(float));
}

void calc_crypto_main(calc_crypto_ctx_t *ctx, const crypto_t *cur, calc_crypto_row_t *row)
{
    // Add current row to circular history buffers (need for calculations) //
    buf_circ_add(&ctx->hist.price, &cur->close);
    buf_circ_add(&ctx->hist.volume, &cur->volume);
    buf_circ_add(&ctx->hist.liq_bid, &cur->liq_bid);

    // Convert timestamp to local date and time //
    struct tm tm;
    localtime_r((time_t *)&cur->ts, &tm);

    // Calculate indicators (step 1) //
    row->rsi = calc_crypto_rsi(ctx);
    row->tail = calc_crypto_tail(ctx);
    row->slope = calc_crypto_slope(ctx);
    row->liquidity = cur->liq_bid + cur->liq_ask;
    row->volume = cur->volume;
    row->volume_surge = calc_crypto_volume_surge(ctx);
    if(ctx->prev_volume_surge > 0) {
        row->volume_accel = row->volume_surge - ctx->prev_volume_surge;
    } else {
        row->volume_accel = 0.0f;
    }
    if(row->liquidity > 0) {
        row->ob_delta = (cur->liq_bid - cur->liq_ask) / row->liquidity;
        row->bid_pressure = cur->liq_bid / row->liquidity;
        row->ask_pressure = cur->liq_ask / row->liquidity;
    } else {
        row->ob_delta = 0;
        row->bid_pressure = 0;
        row->ask_pressure = 0;
    }
    if(cur->liq_ask > 0) {
        row->bid_ask_ratio = cur->liq_bid / cur->liq_ask;
        row->bid_ask_diff_pct = (cur->liq_bid - cur->liq_ask) / (cur->liq_ask - 1) * 100.0f;
    } else {
        row->bid_ask_ratio = 0;
        row->bid_ask_diff_pct = 0;
    }

    // Add calculated indicators to history buffers //
    buf_circ_add(&ctx->hist.rsi, &row->rsi);
    ctx->prev_volume_surge = row->volume_surge;

    // Calculate indicators (step 2) //
    row->hour_of_day = tm.tm_hour;
    row->minute_of_day = tm.tm_min;
    row->price_change_3 = calc_crypto_pct(&ctx->hist.price, PERIOD_3);
    row->price_change_10 = calc_crypto_pct(&ctx->hist.price, PERIOD_10);
    row->price_volatility_10 = calc_crypto_price_volatility(ctx, PERIOD_10);
    row->price_slope_15_pct = calc_crypto_pct(&ctx->hist.price, PERIOD_15) / PERIOD_15;
    row->rsi_change_5 = calc_crypto_pct(&ctx->hist.rsi, PERIOD_5);
    row->rsi_slope_10 = calc_crypto_pct(&ctx->hist.rsi, PERIOD_10) / PERIOD_10;
    row->volume_change_5 = calc_crypto_pct(&ctx->hist.volume, PERIOD_5);
    row->volume_ma_ratio = clac_crypto_volume_ma_ratio(ctx);
    row->liq_bid_growth_15 = calc_crypto_pct(&ctx->hist.liq_bid, PERIOD_15);
}

void calc_crypto(calc_crypto_ctx_t *ctx, const crypto_t *fdata, calc_crypto_row_t *row)
{
    const crypto_t *cur = buf_circ_get(&ctx->hist.forward, ctx->hist.forward.cnt - FCHANGE_PERIOD);
    calc_crypto_main(ctx, cur, row);

    // Check label conditions and collect statistic //
    calc_crypto_fchange_t fchange = calc_crypto_fchange(ctx);
    bool growth_ok = (fchange.growth > GROWTH_MIN && fchange.growth < GROWTH_MAX);
    ctx->stat.growth += growth_ok;

    bool rollback_ok = (fchange.rollback > ROLLBACK_MIN);
    ctx->stat.rollback += rollback_ok;

    bool rsi_ok = (row->rsi < RSI_MIN || row->rsi > RSI_MAX);
    ctx->stat.rsi += rsi_ok;

    bool volume_ok = (cur->volume > VOLUME_MIN);
    ctx->stat.volume += volume_ok;

    bool liq_bid_ok = (cur->liq_bid > LIQ_BID_KOEF * cur->liq_ask);
    ctx->stat.liq_bid += liq_bid_ok;

    bool rsi_change_5_ok = (row->rsi_change_5 > RSI_PCT5_MIN);
    ctx->stat.rsi_change_5 += rsi_change_5_ok;

    bool price_slope_15_pct_ok = (row->price_slope_15_pct > SLOPE_PCT15_MIN);
    ctx->stat.price_slope_15_pct += price_slope_15_pct_ok;

    bool vol_ma_ratio_ok = (row->volume_ma_ratio > VOL_MA_RATIO_MIN);
    ctx->stat.vol_ma_ratio += vol_ma_ratio_ok;

    bool bid_pressure_ok = (row->bid_pressure > LIQ_BID_PRESSURE_MIN);
    ctx->stat.bid_pressure += bid_pressure_ok;

    bool liq_bid_growth_15_ok = (row->liq_bid_growth_15 > LIQ_BID_PCT15_MIN);
    ctx->stat.liq_bid_growth_15 += liq_bid_growth_15_ok;

    uint32_t derived_signals =
            rsi_change_5_ok + price_slope_15_pct_ok + vol_ma_ratio_ok + bid_pressure_ok + liq_bid_growth_15_ok;
    bool derived_signals_ok = (derived_signals >= 3);
    ctx->stat.derived_signals += derived_signals_ok;

    // Primory label //
    bool label1 = true;
    label1 &= growth_ok;
    label1 &= rollback_ok;
    label1 &= rsi_ok;
    label1 &= volume_ok;
    label1 &= liq_bid_ok;
    label1 &= derived_signals_ok;
    row->label1 = label1 & !ctx->prev_label1;
    ctx->prev_label1 = label1;
    ctx->stat.label1 += row->label1;

    // Check secondary label conditions and collect statistic //
    row->change_05 = calc_crypto_fpchange(ctx, cur->close, PERIOD_5);
    row->change_15 = calc_crypto_fpchange(ctx, cur->close, PERIOD_15);
    row->change_30 = calc_crypto_fpchange(ctx, cur->close, PERIOD_30);
    row->change_45 = calc_crypto_fpchange(ctx, cur->close, PERIOD_45);

    bool change_05_ok = (row->change_05 > CHANGE_05_MIN);
    ctx->stat.change_05 += change_05_ok;
    bool change_15_ok = (row->change_15 > CHANGE_15_MIN);
    ctx->stat.change_15 += change_15_ok;
    bool change_30_ok = (row->change_30 > CHANGE_30_MIN);
    ctx->stat.change_30 += change_30_ok;
    bool change_45_ok = (row->change_45 > CHANGE_45_MIN);
    ctx->stat.change_45 += change_45_ok;

    bool label2 = true;
    label2 &= change_05_ok;
    label2 &= change_15_ok;
    label2 &= change_30_ok;
    label2 &= change_45_ok;
    row->label2 = label2 & !ctx->prev_label2;
    ctx->prev_label2 = label2;
    ctx->stat.label2 += row->label2;

    // Final label //
    row->label = row->label1 & row->label2;
    ctx->stat.label += row->label;

    // Advance forward buffer if new data is provided //
    buf_circ_add(&ctx->hist.forward, fdata);
}

void calc_crypto_log_stat(const calc_crypto_stat_t *stat)
{
    log_debug("fchange_cache_cnt: %u", stat->fchange_cache_cnt);
    log_debug("growth_ok: %u", stat->growth);
    log_debug("rollback_ok: %u", stat->rollback);
    log_debug("rsi_ok: %u", stat->rsi);
    log_debug("volume_ok: %u", stat->volume);
    log_debug("liq_bid_ok: %u", stat->liq_bid);
    log_debug("rsi_change_5_ok: %u", stat->rsi_change_5);
    log_debug("price_slope_15_pct_ok: %u", stat->price_slope_15_pct);
    log_debug("vol_ma_ratio_ok: %u", stat->vol_ma_ratio);
    log_debug("bid_pressure_ok: %u", stat->bid_pressure);
    log_debug("liq_bid_growth_15_ok: %u", stat->liq_bid_growth_15);
    log_debug("derived_signals_ok: %u", stat->derived_signals);
    log_debug("label1: %u", stat->label1);
    log_debug("change_05_ok: %u", stat->change_05);
    log_debug("change_15_ok: %u", stat->change_15);
    log_debug("change_30_ok: %u", stat->change_30);
    log_debug("change_45_ok: %u", stat->change_45);
    log_debug("label2: %u", stat->label2);
    log_debug("label: %u", stat->label);
}
