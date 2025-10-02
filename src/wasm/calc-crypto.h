#pragma once

#include <stdint.h>

#define PRICES_COUNT 15
#define RSI_PERIOD   14
#define TAIL_PERIOD  5
#define SLOPE_PERIOD 10
_Static_assert(RSI_PERIOD + 1 <= PRICES_COUNT, "RSI_PERIOD + 1 must be less than or equal to PRICES_COUNT");
_Static_assert(TAIL_PERIOD <= PRICES_COUNT, "TAIL_PERIOD must be less than or equal to PRICES_COUNT");
_Static_assert(SLOPE_PERIOD <= PRICES_COUNT, "SLOPE_PERIOD must be less than or equal to PRICES_COUNT");

#define VOLUME_SURGE_COUNT 5

/**
 * @brief Calculate the Relative Strength Index (RSI) for a given array of prices.
 * @param prices - [in] Circular array of price data
 * @param count - [in] Number of price data points
 * @param offset - [in] Offset to start the RSI calculation
 * @return The calculated RSI value
 */
float calc_crypto_rsi(const float *prices, uint32_t count, uint32_t offset);

/**
 * @brief Calculate the tail size for a given array of prices.
 * @param prices - [in] Circular array of price data
 * @param count - [in] Number of price data points
 * @param offset - [in] Offset to start the tail calculation
 * @return The calculated tail size
 */
float calc_crypto_tail(const float *prices, uint32_t count, uint32_t offset);

/**
 * @brief Calculate the slope for a given array of prices.
 * @param prices - [in] Circular array of price data
 * @param count - [in] Number of price data points
 * @param offset - [in] Offset to start the slope calculation
 * @return The calculated slope value
 */
float calc_crypto_slope(const float *prices, uint32_t count, uint32_t offset);

/**
 * @brief Calculate the volume surge for a given array of volumes.
 * @param volumes - [in] Circular array of volume data
 * @param count - [in] Number of volume data points
 * @param offset - [in] Offset to start the volume surge calculation
 * @param current_volume - [in] The current volume to compare against
 * @return The calculated volume surge value
 */
float calc_crypto_volume_surge(const float *volumes, uint32_t count, uint32_t offset, float current_volume);
