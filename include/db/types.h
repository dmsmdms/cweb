#pragma once

#include <core/db.h>

/**
 * @brief Database table identifiers
 */
typedef enum PACKED {
    DB_TABLE_NONE,
    DB_TABLE_JOB_LT,     ///< Table where key - db_key_id_t and value - db_job_value_t
    DB_TABLE_JOB_LT_URL, ///< Table where key - db_key_url_t and value - db_key_id_t
    DB_TABLE_MAX,
} db_table_t;

/**
 * @brief First key in each table, stores metadata about the table in value
 */
typedef struct PACKED {
    db_table_t table; ///< Table identifier
} db_key_meta_t;

/**
 * @brief Key for tables where key is ID
 */
typedef struct PACKED {
    db_table_t table; ///< Table identifier
    uint32_t id : 24; ///< Autoincrement unique ID
} db_key_id_t;

/**
 * @brief Key for tables where key is URL
 */
typedef struct PACKED {
    db_table_t table;  ///< Table identifier
    uint8_t url_len;   ///< Length of the URL string
    char url[256 - 2]; ///< URL (null-terminated)
} db_key_url_t;

/**
 * @brief Get metadata for a specific table
 * @param db - [in] Pointer to the database instance
 * @param table - [in] Table identifier
 * @return Pointer to the metadata structure, or NULL on failure
 */
const void *db_get_meta(const db_t *db, db_table_t table);

/**
 * @brief Get value by ID
 * @param db - [in] Pointer to the database instance
 * @param table - [in] Table identifier with ID keys
 * @param id - [in] ID key
 * @return Pointer to the value structure, or NULL if not found
 */
const void *db_get_value_by_id(const db_t *db, db_table_t table, uint32_t id);

/**
 * @brief Get ID key by URL
 * @param db - [in] Pointer to the database instance
 * @param table - [in] Table identifier with URL keys
 * @param url - [in] URL string
 * @return Pointer to the ID key structure, or NULL if not found
 */
const db_key_id_t *db_get_id_by_url(const db_t *db, db_table_t table, const char *url);

/**
 * @brief Put metadata for a specific table
 * @param db - [in] Pointer to the database instance
 * @param table - [in] Table identifier
 * @param meta - [in] Pointer to the metadata structure
 * @param size - [in] Size of the metadata structure
 * @return True on success, false on failure
 */
bool db_put_meta(const db_t *db, db_table_t table, const void *meta, size_t size);

/**
 * @brief Put value by ID
 * @param db - [in] Pointer to the database instance
 * @param table - [in] Table identifier with ID keys
 * @param id - [in] ID key
 * @param value - [in] Pointer to the value data
 * @param size - [in] Size of the value data
 * @return True on success, false on failure
 */
bool db_put_value_by_id(db_t *db, db_table_t table, uint32_t id, const void *value, size_t size);

/**
 * @brief Put ID key by URL
 * @param db - [in] Pointer to the database instance
 * @param url_table - [in] Table identifier with URL keys
 * @param url - [in] URL string
 * @param id_table - [in] Table identifier with ID keys
 * @param id - [in] ID key to associate with the URL
 * @return True on success, false on failure
 */
bool db_put_id_by_url(db_t *db, db_table_t url_table, const char *url, db_table_t id_table, uint32_t id);
