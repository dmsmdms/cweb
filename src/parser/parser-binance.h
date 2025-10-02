#pragma once

#include <core/json/json-parser.h>
#include <core/ws/ws-client.h>
#include <db/db-crypto.h>

#define BINANCE_LIQ_COUNT   10
#define BINANCE_MAX_STREAMS 80
#define BINANCE_MAX_CONNS   ((CRYPTO_MAX_SYMBOLS + BINANCE_MAX_STREAMS - 1) / BINANCE_MAX_STREAMS)

/**
 * @brief Binance stream types
 */
typedef enum {
    BINANCE_STREAM_DEPTH, ///< Depth update
    BINANCE_STREAM_KLINE, ///< Kline/candlestick update
    BINANCE_STREAM_MAX,
} binance_stream_t;

/**
 * @brief Binance kline data structure
 */
typedef struct {
    float close_price; ///< Close price
    float volume;      ///< Volume
} binance_kline_t;

/**
 * @brief Binance depth data indices
 */
typedef enum {
    BINANCE_DEPTH_PRM_PRICE, ///< Price index
    BINANCE_DEPTH_PRM_QTY,   ///< Quantity index
    BINANCE_DEPTH_PRM_MAX,
} binance_depth_prm_t;

/**
 * @brief Binance liquidation entry structure
 */
typedef struct {
    float data[BINANCE_DEPTH_PRM_MAX]; ///< [price, quantity]
    uint32_t count;                    ///< Number of entries
} binance_liq_entry_t;

/**
 * @brief Binance liquidation data structure
 */
typedef struct {
    binance_liq_entry_t data[BINANCE_LIQ_COUNT]; ///< Array of liquidation data
    uint32_t count;                              ///< Number of entries
} binance_liq_t;

/**
 * @brief Binance depth data structure
 */
typedef struct {
    binance_liq_t bids; ///< Bids [price, quantity]
    binance_liq_t asks; ///< Asks [price, quantity]
} binance_depth_t;

/**
 * @brief Union of Binance data types
 */
typedef union {
    binance_kline_t kline; ///< Kline data
    binance_depth_t depth; ///< Depth data
} binance_data_t;

/**
 * @brief Binance update structure
 */
typedef struct {
    const char *symbol;    ///< Trading symbol
    binance_stream_t type; ///< Stream type
    binance_data_t data;   ///< Stream data
} binance_update_t;

typedef struct {
    char symbol[12];  ///< Trading symbol
    float liq_avg;    ///< Total liquidation volume
    float liq_ask;    ///< Liquidation ask
    float liq_bid;    ///< Liquidation bid
    uint32_t liq_cnt; ///< Number of liquidation trades
    uint32_t whales;  ///< Number of whale trades
} binance_depth_val_t;

/**
 * @brief Binance parser structure
 */
typedef struct {
    struct hsearch_data depth_htab;      ///< Depth hash table
    binance_depth_val_t *depth_arr;      ///< Array of depth values
    ws_conn_t *conns[BINANCE_MAX_CONNS]; ///< WebSocket connections
    uint32_t symbol_cnt;                 ///< Number of symbols
} parser_binance_t;

/**
 * @brief Initialize Binance parser
 * @param app - [in] Pointer to the application structure
 * @return True on success, false on failure
 */
bool parser_binance_init(app_t *app);

/**
 * @brief Destroy Binance parser
 * @param app - [in] Pointer to the application structure
 */
void parser_binance_destroy(app_t *app);

/**
 * @brief Parse Binance stream JSON
 * @param app - [in] Pointer to the application structure
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Pointer to private data (if any)
 * @return True on success, false on failure
 */
bool parser_binance_stream(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data);

/**
 * @brief Parse Binance data JSON
 * @param app - [in] Pointer to the application structure
 * @param cur - [in] Current JSON token
 * @param json - [in] Original JSON string
 * @param priv_data - [in] Pointer to private data (if any)
 * @return True on success, false on failure
 */
bool parser_binance_data(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data);
