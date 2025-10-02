#pragma once

#include <core/db/db.h>

/**
 * @brief Structure to hold cryptocurrency metadata
 */
typedef struct {
    uint32_t sym_id_last; ///< Last cryptocurrency symbol ID
    uint32_t sym_count;   ///< Number of cryptocurrency symbols
} db_crypto_meta_t;

/**
 * @brief Structure to hold cryptocurrency data in the database
 */
typedef struct {
    float close;    ///< Closing price
    float volume;   ///< Trading volume
    float liq_ask;  ///< Liquidity on the ask side
    float liq_bid;  ///< Liquidity on the bid side
    uint8_t whales; ///< Number of whale transactions
} db_crypto_t;

/**
 * @brief Retrieve cryptocurrency metadata from the database
 * @param meta - [out] Pointer to store retrieved metadata
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_crypto_get_meta(db_crypto_meta_t *meta);

/**
 * @brief Put cryptocurrency metadata in the database
 * @param meta - [in] Pointer to metadata to be updated
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_crypto_put_meta(const db_crypto_meta_t *meta);

/**
 * @brief Retrieve cryptocurrency symbol by ID
 * @param psym - [out] Pointer to store the cryptocurrency symbol string
 * @param psym_size - [out] Pointer to store the size of the cryptocurrency symbol string
 * @param psym_id - [out] Pointer to store the cryptocurrency symbol ID
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_crypto_get_sym_next(const char **psym, uint32_t *psym_size, uint32_t *psym_id);

/**
 * @brief Put cryptocurrency symbol in the database
 * @param sym - [in] Cryptocurrency symbol string
 * @param sym_id - [in] Cryptocurrency symbol ID
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_crypto_put_sym(const char *sym, uint32_t sym_id);

/**
 * @brief Put cryptocurrency data in the database
 * @param sym_id - [in] Cryptocurrency symbol ID
 * @param ts - [in] Timestamp of the cryptocurrency data
 * @param crypto - [in] Pointer to cryptocurrency data to be added
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_crypto_put(uint32_t sym_id, uint64_t ts, const db_crypto_t *crypto);
