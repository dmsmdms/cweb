#pragma once

#include <db/db-crypto.h>

/**
 * @brief Structure to hold cryptocurrency metadata
 */
typedef struct {
    uint32_t sym_id_last; ///< Last cryptocurrency symbol ID
    uint32_t sym_count;   ///< Number of glob cryptocurrency symbols
} db_crypto_meta_t;

typedef struct {
    uint32_t id;    ///< Cryptocurrency symbol ID
    bool is_glob;   ///< Flag indicating if the symbol is global
    bool is_loc;    ///< Flag indicating if the symbol is local
    uint8_t pad[2]; ///< Padding for alignment
} db_crypto_sym_t;

/**
 * @brief Structure to hold cryptocurrency data in the database
 */
typedef struct {
    float close;    ///< Closing price
    float volume;   ///< Trading volume
    float liq_ask;  ///< Liquidity on the ask side
    float liq_bid;  ///< Liquidity on the bid side
    uint8_t whales; ///< Number of whale transactions
    uint8_t pad[3]; ///< Padding for alignment
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
 * @brief Retrieve cryptocurrency symbol ID by symbol string
 * @param sym_name - [in] Cryptocurrency symbol string
 * @param sym - [out] Pointer to store the cryptocurrency symbol ID
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_crypto_get_sym(const char *sym_name, db_crypto_sym_t *sym);

/**
 * @brief Retrieve cryptocurrency symbol by ID
 * @param psym_name - [out] Pointer to store the cryptocurrency symbol string
 * @param psym_name_size - [out] Pointer to store the size of the cryptocurrency symbol string
 * @param sym - [out] Pointer to store the cryptocurrency symbol ID
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_crypto_get_sym_next(const char **psym_name, uint32_t *psym_name_size, db_crypto_sym_t *sym);

/**
 * @brief Put cryptocurrency symbol in the database
 * @param sym_name - [in] Cryptocurrency symbol string
 * @param sym - [in] Pointer to cryptocurrency symbol ID
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_crypto_put_sym(const char *sym_name, const db_crypto_sym_t *sym);

/**
 * @brief Retrieve next cryptocurrency data by symbol ID
 * @param min_sym_id - [in] Minimum symbol ID to consider
 * @param max_sym_id - [in] Maximum symbol ID to consider
 * @param min_ts - [in] Minimum timestamp to consider
 * @param max_ts - [in] Maximum timestamp to consider
 * @param pts - [out] Pointer to store the timestamp of the cryptocurrency data
 * @param crypto - [out] Pointer to store the retrieved cryptocurrency data
 * @param op - [in] Cursor operation (e.g., NEXT, PREV)
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_crypto_get_next(uint32_t min_sym_id, uint32_t max_sym_id, uint64_t min_ts, uint64_t max_ts, uint64_t *pts,
                            db_crypto_t *crypto, db_cursor_op_t op);

/**
 * @brief Retrieve next cryptocurrency data for a specific symbol ID
 * @param sym_id - [in] Cryptocurrency symbol ID
 * @param op - [in] Cursor operation (e.g., NEXT, PREV)
 * @param crypto - [out] Pointer to store the retrieved cryptocurrency data
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_crypto_get_next1(uint32_t sym_id, db_cursor_op_t op, crypto_t *crypto);

/**
 * @brief Put cryptocurrency data in the database
 * @param sym_id - [in] Cryptocurrency symbol ID
 * @param ts - [in] Timestamp of the cryptocurrency data
 * @param crypto - [in] Pointer to cryptocurrency data to be added
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_crypto_put(uint32_t sym_id, uint64_t ts, const db_crypto_t *crypto);
