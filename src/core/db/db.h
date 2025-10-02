#pragma once

#include <common.h>
#include <lmdb.h>

/**
 * @brief Instance of the database handler
 */
typedef struct {
    MDB_env *env;
    MDB_txn *txn;
    MDB_dbi dbi;
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
