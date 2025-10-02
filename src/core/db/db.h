#pragma once

#include <common.h>
#include <lmdb.h>

/**
 * @brief Instance of the database handler
 */
typedef struct {
    MDB_env *env;    ///< LMDB environment
    MDB_txn *txn;    ///< LMDB transaction
    MDB_cursor *cur; ///< LMDB cursor
    MDB_dbi dbi;     ///< LMDB database instance
} db_t;

/**
 * @brief Create or open database in specified path
 * @param app - [in] Pointer to the application instance
 * @return true on success, false otherwise
 */
bool db_open(app_t *app);

/**
 * @brief Close database and free resources
 * @param app - [in] Pointer to the application instance
 */
void db_close(app_t *app);

/**
 * @brief Begin a new transaction
 * @param app - [in] Pointer to the application instance
 * @return true on success, false otherwise
 */
bool db_txn_begin(app_t *app);

/**
 * @brief Get data for the specified key using a cursor
 * @param app - [in] Pointer to the application instance
 * @param key - [in] Pointer to the key value
 * @param data - [out] Pointer to the data value
 * @param op - [in] Cursor operation
 * @return True on success, false otherwise
 */
bool db_cursor_get(app_t *app, const MDB_val *key, MDB_val *data, MDB_cursor_op op);

/**
 * @brief Commit current transaction
 * @param app - [in] Pointer to the application instance
 * @return true on success, false otherwise
 */
bool db_txn_commit(app_t *app);

/**
 * @brief Abort current transaction
 * @param app - [in] Pointer to the application instance
 */
void db_txn_abort(app_t *app);
