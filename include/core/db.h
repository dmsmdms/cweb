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
 * @brief Create or open database in specified directory
 * @param db - [out] instance of the database handler
 * @param dir - [in] directory where database files will be stored
 * @return true on success, false otherwise
 */
bool db_open(db_t *db, const char *dir);

/**
 * @brief Close database and free resources
 * @param db - [in] instance of the database handler
 */
void db_close(db_t *db);
