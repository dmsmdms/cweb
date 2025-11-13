#pragma once

#include <core/db/db.h>

#define CRYPTO_SYM_ARR_BUF_SIZE (128 * 1024)

/**
 * @brief Structure to hold cryptocurrency data
 */
typedef struct {
    uint64_t ts;     ///< Timestamp
    float close;     ///< Closing price
    float volume;    ///< Volume
    float liq_ask;   ///< Liquidation ask
    float liq_bid;   ///< Liquidation bid
    uint32_t whales; ///< Number of whale trades
} crypto_t;

/**
 * @brief Structure to hold an array of cryptocurrency data
 */
typedef struct {
    crypto_t *data; ///< Pointer to the array of cryptocurrency data
    uint32_t count; ///< Number of records in the array
} crypto_arr_t;

/**
 * @brief Structure to hold cryptocurrency symbol name
 */
typedef struct {
    const char *name; ///< Cryptocurrency symbol name
    uint32_t id;      ///< Cryptocurrency symbol ID
} crypto_sym_t;

/**
 * @brief Structure to hold an array of cryptocurrency symbols
 */
typedef struct {
    crypto_sym_t *data; ///< Pointer to the array of cryptocurrency symbols
    uint32_t count;     ///< Number of symbols
} crypto_sym_arr_t;

/**
 * @brief Initialize the cryptocurrency database
 * @param crypto_list_path - [in] Path to the file containing the list of cryptocurrency symbols
 * @return ERR_DB_OK on success, error code on failure
 */
db_err_t db_crypto_init(const char *crypto_list_path);

/**
 * @brief Import cryptocurrency data from a CSV file into the database
 * @param csv_path - [in] Path to the CSV file
 * @param sym_name - [in] Name of the cryptocurrency symbol
 * @return ERR_DB_OK on success, error code on failure
 */
db_err_t db_crypto_import_csv(const char *csv_path, const char *sym_name);

/**
 * @brief Export cryptocurrency data from the database to a CSV file
 * @param csv_path - [in] Path to the CSV file
 * @param sym_name - [in] Name of the cryptocurrency symbol
 * @return ERR_DB_OK on success, error code on failure
 */
db_err_t db_crypto_export_csv(const char *csv_path, const char *sym_name);

/**
 * @brief Export calculated cryptocurrency data to a CSV file
 * @param csv_path - [in] Path to the CSV file
 * @param sym_name - [in] Name of the cryptocurrency symbol
 * @return ERR_DB_OK on success, error code on failure
 */
db_err_t db_crypto_export_calc_csv(const char *csv_path, const char *sym_name);

/**
 * @brief Add a cryptocurrency symbol to the database
 * @param sym_id - [in] ID of the cryptocurrency symbol
 * @param crypto - [in] Pointer to the cryptocurrency data structure
 * @return ERR_DB_OK on success, error code on failure
 */
db_err_t db_crypto_add(uint32_t sym_id, const crypto_t *crypto);

/**
 * @brief Get cryptocurrency symbols from the database
 * @param arr - [out] Pointer to the array to hold the cryptocurrency symbols
 * @param buf - [in] Buffer for temporary storage
 * @return ERR_DB_OK on success, error code on failure
 */
db_err_t db_crypto_sym_arr_get(crypto_sym_arr_t *arr, buf_ext_t *buf);

/**
 * @brief Train AI model for cryptocurrency prediction
 * @param path - [in] Path to save the trained model
 * @param sym_name - [in] Name of the cryptocurrency symbol
 * @return ERR_DB_OK on success, error code on failure
 */
db_err_t db_crypto_ai_train_model(const char *path, const char *sym_name);

/**
 * @brief Deinitialize the cryptocurrency AI module
 */
void db_crypto_ai_deinit(void);
