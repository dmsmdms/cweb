#include <calc/calc-crypto-func.h>
#include <core/base/log.h>
#include <strings.h>

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

void calc_crypto_init(calc_crypto_ctx_t *ctx)
{
    bzero(&ctx->stat, sizeof(crypto_calc_stat_t));
    ctx->prev_label1 = false;
    ctx->prev_label2 = false;

    buf_circ_init(&ctx->forward, ctx->forward_buf, CALC_CRYPTO_SIZE_FORWARD, sizeof(crypto_t));
    buf_circ_init(&ctx->price_hist, ctx->price_hist_buf, CALC_CRYPTO_SIZE_PRICE_HIST, sizeof(float));
    buf_circ_init(&ctx->volume_hist, ctx->volume_hist_buf, CALC_CRYPTO_SIZE_VOLUME_HIST, sizeof(float));
    buf_circ_init(&ctx->liq_bid_hist, ctx->liq_bid_hist_buf, CALC_CRYPTO_SIZE_LIQ_HIST, sizeof(float));
    buf_circ_init(&ctx->rsi_hist, ctx->rsi_hist_buf, CALC_CRYPTO_SIZE_RSI_HIST, sizeof(float));
    buf_circ_init(&ctx->slope_hist, ctx->slope_hist_buf, CALC_CRYPTO_SIZE_SLOPE_HIST, sizeof(float));
}

void calc_crypto(calc_crypto_ctx_t *ctx, const crypto_t *fdata, crypto_calc_row_t *row)
{
    const crypto_t *cur = buf_circ_get(&ctx->forward, 0);
    buf_circ_add(&ctx->price_hist, &cur->close);
    buf_circ_add(&ctx->volume_hist, &cur->volume);
    buf_circ_add(&ctx->liq_bid_hist, &cur->liq_bid);

    // Calculate indicators //
    calc_crypto_fchange_t fchange = calc_crypto_fchange(ctx);
    row->ts = cur->ts;
    row->close = cur->close;
    row->volume = cur->volume;
    row->liq_ask = cur->liq_ask;
    row->liq_bid = cur->liq_bid;
    row->whales = cur->whales;

    row->rsi = calc_crypto_rsi(ctx);
    buf_circ_add(&ctx->rsi_hist, &row->rsi);
    row->slope = calc_crypto_slope(ctx);
    buf_circ_add(&ctx->slope_hist, &row->slope);
    row->liquidity = cur->liq_bid + cur->liq_ask;

    row->rsi_pct5 = calc_crypto_pct(&ctx->rsi_hist, 5);
    row->slope_pct15 = calc_crypto_pct(&ctx->slope_hist, 15);
    row->volume_ma_ratio = clacl_crypto_volume_ma_ratio(ctx);
    row->bid_pressure = row->liquidity ? (cur->liq_bid / row->liquidity) : 0.0f;
    row->liq_bid_pct15 = calc_crypto_pct(&ctx->liq_bid_hist, 15);

    // Check label conditions //
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

    bool rsi_pct5_ok = (row->rsi_pct5 > RSI_PCT5_MIN);
    ctx->stat.rsi_pct5 += rsi_pct5_ok;
    bool slope_pct15_ok = (row->slope_pct15 > SLOPE_PCT15_MIN);
    ctx->stat.slope_pct15 += slope_pct15_ok;
    bool vol_ma_ratio_ok = (row->volume_ma_ratio > VOL_MA_RATIO_MIN);
    ctx->stat.vol_ma_ratio += vol_ma_ratio_ok;
    bool bid_pressure_ok = (row->bid_pressure > LIQ_BID_PRESSURE_MIN);
    ctx->stat.liq_bid_pressure += bid_pressure_ok;
    bool liq_bid_pct15_ok = (row->liq_bid_pct15 > LIQ_BID_PCT15_MIN);
    ctx->stat.liq_bid_pct15 += liq_bid_pct15_ok;

    uint32_t derived_signals = rsi_pct5_ok + slope_pct15_ok + vol_ma_ratio_ok + bid_pressure_ok + liq_bid_pct15_ok;
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

    // Check label changes //
    row->change_05 = calc_crypto_fpchange(ctx, cur->close, 5);
    row->change_15 = calc_crypto_fpchange(ctx, cur->close, 15);
    row->change_30 = calc_crypto_fpchange(ctx, cur->close, 30);
    row->change_45 = calc_crypto_fpchange(ctx, cur->close, 45);

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

    if(fdata) {
        buf_circ_add(&ctx->forward, fdata);
    } else {
        buf_circ_del_cur(&ctx->forward);
    }
}

void calc_crypto_log_stat(const crypto_calc_stat_t *stat)
{
    log_debug("growth_ok: %u", stat->growth);
    log_debug("rollback_ok: %u", stat->rollback);
    log_debug("rsi_ok: %u", stat->rsi);
    log_debug("volume_ok: %u", stat->volume);
    log_debug("liq_bid_ok: %u", stat->liq_bid);
    log_debug("rsi_pct5_ok: %u", stat->rsi_pct5);
    log_debug("slope_pct15_ok: %u", stat->slope_pct15);
    log_debug("vol_ma_ratio_ok: %u", stat->vol_ma_ratio);
    log_debug("liq_bid_pressure_ok: %u", stat->liq_bid_pressure);
    log_debug("liq_bid_pct15_ok: %u", stat->liq_bid_pct15);
    log_debug("derived_signals_ok: %u", stat->derived_signals);
    log_debug("label1: %u", stat->label1);
    log_debug("change_05_ok: %u", stat->change_05);
    log_debug("change_15_ok: %u", stat->change_15);
    log_debug("change_30_ok: %u", stat->change_30);
    log_debug("change_45_ok: %u", stat->change_45);
    log_debug("label2: %u", stat->label2);
    log_debug("label: %u", stat->label);
}
