#pragma once

#include <common.h>

/**
 * @brief Database table identifiers
 */
typedef enum PACKED {
    DB_TABLE_NONE,
    DB_TABLE_JOB_LT,     ///< Table where key - ID and value - job info
    DB_TABLE_JOB_LT_URL, ///< Table where key - url and value - ID form DB_TABLE_JOB_LT
    DB_TABLE_MAX,
} db_table_t;

/**
 * @brief Instance of the database handler
 */
typedef struct {
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
