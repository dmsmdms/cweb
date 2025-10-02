#pragma once

#include <core/base/buf.h>

/**
 * @brief Enumeration of database error codes
 */
typedef enum {
    DB_ERR_OK,            ///< No error
    DB_ERR_ENV_CREATE,    ///< Environment creation error
    DB_ERR_ENV_INFO,      ///< Environment info error
    DB_ERR_OPEN,          ///< Database open error
    DB_ERR_SYNC,          ///< Database sync error
    DB_ERR_TXN_BEGIN,     ///< Transaction begin error
    DB_ERR_TXN_COMMIT,    ///< Transaction commit error
    DB_ERR_DBI_OPEN,      ///< Database instance open error
    DB_ERR_DBI_GET,       ///< Database instance get error
    DB_ERR_DBI_PUT,       ///< Database instance put error
    DB_ERR_NOT_FOUND,     ///< Database value not found
    DB_ERR_SIZE_MISMATCH, ///< Size mismatch error
    DB_ERR_PARSE,         ///< Parsing error
    DB_ERR_NO_MEM,        ///< Memory allocation error
    DB_ERR_FAIL,          ///< General failure
    DB_ERR_MAX,
} db_err_t;

/**
 * @brief Enumeration of database cursor operations
 */
typedef enum {
    DB_CURSOR_OP_NEXT = 8,       ///< Move cursor to the next key
    DB_CURSOR_OP_SET_RANGE = 17, ///< Set cursor to a specific key
    DB_CURSOR_OP_MAX,
} db_cursor_op_t;

/**
 * @brief Database statistics structure
 */
typedef struct {
    size_t used_size; ///< Used size in bytes
    size_t tot_size;  ///< Total size in bytes
} db_stat_t;

/**
 * @brief Create or open database in specified path
 * @param path - [in] Path to the database directory
 * @param size_mb - [in] Size of the database in megabytes
 * @param max_dbs - [in] Maximum number of databases
 * @param rd_only - [in] Open database in read-only mode if true
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_open(const char *path, uint32_t size_mb, uint32_t max_dbs, bool rd_only);

/**
 * @brief Get database statistics
 * @param stat - [out] Pointer to the statistics structure
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_get_stat(db_stat_t *stat);

/**
 * @brief Sync database to disk
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_sync(void);

/**
 * @brief Close database and free resources
 */
void db_close(void);

/**
 * @brief Begin a new transaction
 * @param rd_only - [in] Begin read-only transaction if true
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_txn_begin(bool rd_only);

/**
 * @brief Commit current transaction
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_txn_commit(void);

/**
 * @brief Abort current transaction
 */
void db_txn_abort(void);

/**
 * @brief Get value from the database within a transaction
 * @param db_name - [in] Name of the database
 * @param key - [in] Pointer to the key buffer
 * @param value - [out] Pointer to the value buffer
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_get(const char *db_name, const buf_t *key, buf_t *value);

/**
 * @brief Put key-value pair into the database within a transaction
 * @param db_name - [in] Name of the database
 * @param key - [in] Pointer to the key buffer
 * @param value - [in] Pointer to the value buffer
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_put(const char *db_name, const buf_t *key, const buf_t *value);

/**
 * @brief Get next key-value pair from the database cursor within a transaction
 * @param db_name - [in] Name of the database
 * @param key - [in,out] Pointer to the key buffer
 * @param value - [in,out] Pointer to the value buffer
 * @param op - [in] Cursor operation (set or next)
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_cursor_get(const char *db_name, buf_t *key, buf_t *value, db_cursor_op_t op);
