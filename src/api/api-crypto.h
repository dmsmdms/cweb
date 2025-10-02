#pragma once

#include <core/http/http-server.h>
#include <core/json/json-parser.h>
#include <db/db-crypto.h>

/**
 * @brief Crypto API actions
 */
typedef enum {
    API_CRYPTO_ACT_GET_SYMBOLS, ///< Get crypto symbols
    API_CRYPTO_ACT_GET_METRICS, ///< Get crypto metrics
    API_CRYPTO_ACT_MAX,         ///< Maximum action value (invalid)
} api_crypto_act_t;

/**
 * @brief Crypto metrics types
 */
typedef enum {
    API_CRYPTO_METRICS_TIMESTAMP,   ///< Timestamp
    API_CRYPTO_METRICS_CLOSE_PRICE, ///< Close price
    API_CRYPTO_METRICS_VOLUME,      ///< Volume
    API_CRYPTO_METRICS_LIQ_ASK,     ///< Liquidity ask
    API_CRYPTO_METRICS_LIQ_BID,     ///< Liquidity bid
    API_CRYPTO_METRICS_WHALES,      ///< Whales count
    API_CRYPTO_METRICS_MAX,
} api_crypto_metrics_t;

/**
 * @brief Request structure for getting crypto metrics
 */
typedef struct {
    char symbol[CRYPTO_SYMBOL_LEN]; ///< Cryptocurrency symbol
    uint64_t start_date;            ///< Start timestamp
    uint64_t end_date;              ///< End timestamp
    uint32_t interval_sec;          ///< Interval in seconds
    uint32_t limit;                 ///< Maximum number of data points to retrieve
} api_crypto_req_metrics_t;

/**
 * @brief Crypto API request data union
 */
typedef union {
    api_crypto_req_metrics_t metrics; ///< Get crypto metrics request data
} api_crypto_req_data_t;

/**
 * @brief Crypto API request structure
 */
typedef struct {
    api_crypto_act_t act;       ///< Crypto API action
    api_crypto_req_data_t data; ///< Crypto API request data
} api_crypto_req_t;

/**
 * @brief Callback type for generating crypto API responses
 * @param app - [in] Pointer to application structure
 * @param resp - [out] Pointer to HTTP server response structure to be filled
 * @param req - [in] Pointer to crypto API request structure
 */
typedef void (*api_crypto_gen_resp_cb_t)(app_t *app, http_srv_resp_t *resp, const api_crypto_req_t *req);

/**
 * @brief Parse crypto API data
 * @param app - [in] Pointer to application structure
 * @param cur - [in] Pointer to current JSON token
 * @param json - [in] Pointer to JSON string
 * @param priv_data - [in] Pointer to private data (api_crypto_req_t)
 * @return True on success, false on failure
 */
bool api_crypto_parse_data(app_t *app, const jsmntok_t *cur, const char *json, void *priv_data);

/**
 * @brief Crypto API request callback
 * @param req - [in] Pointer to HTTP server request structure
 * @param resp - [out] Pointer to HTTP server response structure to be filled
 */
void api_crypto_cb(const http_srv_req_t *req, http_srv_resp_t *resp);
