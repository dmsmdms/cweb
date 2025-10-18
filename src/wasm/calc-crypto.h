#pragma once

#include <stdint.h>

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
