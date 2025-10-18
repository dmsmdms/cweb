#pragma once

#include <core/db/db-table.h>

#define CRYPTO_SYMBOL_LEN  12
#define CRYPTO_MAX_SYMBOLS 1024

/**
 * @brief Key for cryptocurrency data table
 */
typedef struct PACKED {
    db_table_t table;                   ///< Table identifier
    char symbol[CRYPTO_SYMBOL_LEN + 3]; ///< Cryptocurrency symbol (null-terminated)
    uint64_t timestamp;                 ///< Timestamp (in seconds)
} db_key_crypto_t;
_Static_assert(sizeof(db_key_crypto_t) == 24, "Invalid size of db_key_crypto_t");

/**
 * @brief Metadata for cryptocurrency table
 */
typedef struct PACKED {
    uint16_t sumbol_count;                  ///< Number of cryptocurrency symbols
    char data[16 * CRYPTO_MAX_SYMBOLS - 2]; ///< Symbos offsets and null-terminated strings
} db_crypto_meta_t;
_Static_assert(sizeof(db_crypto_meta_t) == 16 * 1024, "Invalid size of db_crypto_meta_t");

/**
 * @brief Database table for storing cryptocurrency data
 */
typedef struct PACKED {
    float close_price; ///< Closing price
    float volume;      ///< Volume
    float liq_ask;     ///< Liquidation ask
    float liq_bid;     ///< Liquidation bid
    uint8_t whales;    ///< Number of whale trades
    char data[7];      ///< Custom data (padding to make structure 24 bytes)
} db_crypto_t;
_Static_assert(sizeof(db_crypto_t) == 24, "Invalid size of db_crypto_t");

/**
 * @brief Structure to hold cryptocurrency symbols
 */
typedef struct {
    const char *symbols[CRYPTO_MAX_SYMBOLS]; ///< Array of cryptocurrency symbols
    uint32_t count;                          ///< Number of symbols
} crypto_sym_t;

/**
 * @brief Structure to hold cryptocurrency data
 */
typedef struct {
    uint64_t timestamp; ///< Timestamp
    float close_price;  ///< Closing price
    float volume;       ///< Volume
    float liq_ask;      ///< Liquidation ask
    float liq_bid;      ///< Liquidation bid
    uint32_t whales;    ///< Number of whale trades
} crypto_t;

/**
 * @brief Structure to hold an array of cryptocurrency data
 */
typedef struct {
    crypto_t *data; ///< Pointer to the array of cryptocurrency data
    uint32_t count; ///< Number of records in the array
} crypto_arr_t;

/**
 * @brief Export cryptocurrency data to a CSV format
 * @param app - [in] Pointer to the application structure
 * @param out - [out] Pointer to the structure to hold the CSV data
 * @param prev_key - [in] Pointer to the previous key for pagination (must zeroed for the first call)
 * @return True on success, false on failure
 */
bool db_crypto_export_csv(app_t *app, str_t *out, MDB_val *prev_key);

/**
 * @brief Export cryptocurrency data to a CSV file
 * @param app - [in] Pointer to the application structure
 * @param path - [in] Path to the output CSV file
 * @return True on success, false on failure
 */
bool db_crypto_export_csv_file(app_t *app, const char *path);

/**
 * @brief Get cryptocurrency symbols from the database
 * @param app - [in] Pointer to the application structure
 * @param out - [out] Pointer to the structure to hold the symbols
 * @return True on success, false on failure
 */
bool db_crypto_sym_get(app_t *app, crypto_sym_t *out);

/**
 * @brief Get cryptocurrency data from the database
 * @param app - [in] Pointer to the application structure
 * @param arr - [out] Pointer to the array to hold the cryptocurrency data
 * @param symbol - [in] Cryptocurrency symbol
 * @param start_ts - [in] Start timestamp (in seconds)
 * @param end_ts - [in] End timestamp (in seconds)
 * @param interval - [in] Interval in seconds
 * @param limit - [in] Maximum number of records to retrieve
 * @return True on success, false on failure
 */
bool db_crypto_get(app_t *app, crypto_arr_t *arr, const char *symbol, uint64_t start_ts, uint64_t end_ts,
                   uint32_t interval);

/**
 * @brief Add a cryptocurrency symbol to the database
 * @param app - [in] Pointer to the application structure
 * @param symbol - [in] Cryptocurrency symbol
 * @param crypto - [in] Pointer to the cryptocurrency data structure
 * @return True on success, false on failure
 */
bool db_crypto_add(app_t *app, const char *symbol, const crypto_t *crypto);
