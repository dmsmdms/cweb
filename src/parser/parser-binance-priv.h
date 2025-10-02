#pragma once

#include <core/json/json-parser.h>

#define BIN_LIQ_COUNT       10
#define BIN_MAX_STREAMS     80
#define BIN_MAX_WS_PATH_LEN (16 * 1024)
#define BIN_LIQ_HIST        200
#define BIN_LIQ_TRESH       0.2f

/**
 * @brief Binance stream types
 */
typedef enum {
    BIN_STREAM_DEPTH, ///< Depth update
    BIN_STREAM_KLINE, ///< Kline/candlestick update
    BIN_STREAM_MAX,
} bin_stream_t;

/**
 * @brief Binance kline data structure
 */
typedef struct {
    float close;  ///< Close price
    float volume; ///< Volume
} bin_kline_t;

/**
 * @brief Binance depth data indices
 */
typedef enum {
    BIN_DEPTH_PRM_PRICE, ///< Price index
    BIN_DEPTH_PRM_QTY,   ///< Quantity index
    BIN_DEPTH_PRM_MAX,
} bin_depth_prm_t;

/**
 * @brief Binance liquidation entry structure
 */
typedef struct {
    float data[BIN_DEPTH_PRM_MAX]; ///< [price, quantity]
    uint32_t count;                ///< Number of entries
} bin_liq_entry_t;

/**
 * @brief Binance liquidation data structure
 */
typedef struct {
    bin_liq_entry_t data[BIN_LIQ_COUNT]; ///< Array of liquidation data
    uint32_t count;                      ///< Number of entries
} bin_liq_t;

/**
 * @brief Binance depth data structure
 */
typedef struct {
    bin_liq_t bids; ///< Bids [price, quantity]
    bin_liq_t asks; ///< Asks [price, quantity]
} bin_depth_t;

/**
 * @brief Union of Binance data types
 */
typedef union {
    bin_kline_t kline; ///< Kline data
    bin_depth_t depth; ///< Depth data
} bin_data_t;

/**
 * @brief Binance update structure
 */
typedef struct {
    const char *symbol; ///< Trading symbol
    bin_stream_t type;  ///< Stream type
    bin_data_t data;    ///< Stream data
} bin_update_t;

typedef struct {
    float liq_avg;     ///< Total liquidation volume
    float liq_ask;     ///< Liquidation ask
    float liq_bid;     ///< Liquidation bid
    uint32_t liq_cnt;  ///< Number of liquidation trades
    uint32_t whales;   ///< Number of whale trades
    uint32_t sym_id;   ///< Symbol ID
    char sym_name[16]; ///< Trading symbol
} bin_depth_val_t;

/**
 * @brief Parse Binance stream JSON
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Pointer to private data (if any)
 * @return JSON_PARSE_ERR_OK on success, error code on failure
 */
json_parse_err_t parser_bin_stream(const jsmntok_t *cur, const char *json, void *priv_data);

/**
 * @brief Parse Binance data JSON
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Pointer to private data (if any)
 * @return JSON_PARSE_ERR_OK on success, error code on failure
 */
json_parse_err_t parser_bin_data(const jsmntok_t *cur, const char *json, void *priv_data);

/**
 * @brief Calculate kline values from Binance update
 * @param update - [in] Pointer to Binance update
 * @param val - [in] Pointer to depth values structure
 * @param pts - [in] Pointer to timestamp of the kline
 */
void parser_bin_calc_kline(const bin_update_t *update, const bin_depth_val_t *val, uint64_t *pts);

/**
 * @brief Calculate depth values from Binance update
 * @param update - [in] Pointer to Binance update
 * @param val - [out] Pointer to depth values structure
 */
void parser_bin_calc_depth(const bin_update_t *update, bin_depth_val_t *val);
