#pragma once

#include <core/base/str.h>
#include <core/db/db.h>

/**
 * @brief Database table identifiers
 */
typedef enum PACKED {
    DB_TABLE_NONE,
#ifdef CONFIG_DB_JOB_TABLE
    DB_TABLE_JOB,     ///< Table where key - db_key_id_t and value - db_job_value_t
    DB_TABLE_JOB_URL, ///< Table where key - db_key_url_t and value - db_key_id_t
#endif
#ifdef CONFIG_DB_CRYPTO_TABLE
    DB_TABLE_CRYPTO, ///< Table where key - db_key_crypto_t and value - db_crypto_t
#endif
    DB_TABLE_MAX,
} db_table_t;

/**
 * @brief Callback type for iterating over database entries
 * @param app - [in] Pointer to the application instance
 * @param key_data - [in] Pointer to the key data
 * @param value_data - [in] Pointer to the value data
 * @param value_size - [in] Size of the value data
 * @param priv_data - [in] Pointer to private data passed to the callback
 * @return True to continue iteration, false to stop
 */
typedef bool (*db_iterate_cb_t)(app_t *app, const void *key_data, const void *value_data, size_t value_size,
                                void *priv_data);

/**
 * @brief Callback type for exporting database entries
 * @param app - [in] Pointer to the application instance
 * @param out - [in,out] Pointer to the output string
 * @param mdb_key - [in] Pointer to the MDB key
 * @param mdb_value - [in] Pointer to the MDB value
 * @param priv_data - [in] Pointer to private data passed to the callback
 * @return True on success, false on failure
 */
typedef bool (*db_export_cb_t)(app_t *app, str_t *out, const MDB_val *mdb_key, const MDB_val *mdb_value,
                               void *priv_data);

/**
 * @brief First key in each table, stores metadata about the table in value
 */
typedef struct PACKED {
    db_table_t table; ///< Table identifier
    char reserved[3]; ///< Reserved for alignment
} db_key_meta_t;
_Static_assert(sizeof(db_key_meta_t) == 4, "Invalid size of db_key_meta_t");

/**
 * @brief Key for tables where key is ID
 */
typedef struct PACKED {
    db_table_t table; ///< Table identifier
    char reserved[3]; ///< Reserved for alignment
    uint32_t id;      ///< Autoincrement unique ID
} db_key_id_t;
_Static_assert(sizeof(db_key_id_t) == 8, "Invalid size of db_key_id_t");

/**
 * @brief Key for tables where key is URL
 */
typedef struct PACKED {
    db_table_t table;   ///< Table identifier
    uint16_t url_len;   ///< Length of the URL string
    char url[1024 - 3]; ///< URL (null-terminated)
} db_key_url_t;
_Static_assert(sizeof(db_key_url_t) == 1024, "Invalid size of db_key_url_t");

/**
 * @brief Put key-value pair into the database
 * @param app - [in] Pointer to the application instance
 * @param type - [in] Type of the key-value pair (for logging purposes)
 * @param key_data - [in] Pointer to the key data
 * @param key_size - [in] Size of the key data
 * @param value_data - [in] Pointer to the value data
 * @param value_size - [in] Size of the value data
 * @return True on success, false on failure
 */
bool db_put(app_t *app, const char *type, const void *key_data, size_t key_size, const void *value_data,
            size_t value_size);

/**
 * @brief Get value by key from the database
 * @param app - [in] Pointer to the application instance
 * @param type - [in] Type of the key-value pair (for logging purposes)
 * @param key_min - [in] Pointer to minimum key data
 * @param key_max - [in] Pointer to maximum key data
 * @param key_size - [in] Size of the key data
 * @param cb - [in] Callback function to process the retrieved value
 * @param priv_data - [in] Pointer to private data passed to the callback
 * @return True on success, false on failure
 */
bool db_iterate(app_t *app, const char *type, const void *key_min, const void *key_max, size_t key_size,
                db_iterate_cb_t cb, void *priv_data);

/**
 * @brief Export database entries of a specific type
 * @param app - [in] Pointer to the application instance
 * @param type - [in] Type of the entries to export
 * @param out - [in,out] Pointer to the output string
 * @param cb - [in] Callback function to process each entry
 * @param priv_data - [in] Pointer to private data passed to the callback
 * @return True on success, false on failure
 */
bool db_export(app_t *app, const char *type, str_t *out, db_export_cb_t cb, MDB_val *prev_key, void *priv_data);

/**
 * @brief Create metadata for a specific table if it does not exist
 * @param app - [in] Pointer to the application instance
 * @param table - [in] Table identifier
 * @param def_value - [in] Pointer to default meta value (added if meta not found)
 * @param def_size - [in] Size of the default meta value
 * @return Pointer to the metadata structure, or NULL on failure
 */
const void *db_create_meta(app_t *app, db_table_t table, const void *def_value, size_t def_size);

/**
 * @brief Get metadata for a specific table
 * @param app - [in] Pointer to the application instance
 * @param table - [in] Table identifier
 * @return Pointer to the metadata structure, or NULL on failure
 */
const void *db_get_meta(app_t *app, db_table_t table);

/**
 * @brief Get value by ID
 * @param app - [in] Pointer to the application instance
 * @param table - [in] Table identifier with ID keys
 * @param id - [in] ID key
 * @return Pointer to the value structure, or NULL if not found
 */
const void *db_get_value_by_id(app_t *app, db_table_t table, uint32_t id);

/**
 * @brief Get ID key by URL
 * @param app - [in] Pointer to the application instance
 * @param table - [in] Table identifier with URL keys
 * @param url - [in] URL string
 * @return Pointer to the ID key structure, or NULL if not found
 */
const db_key_id_t *db_get_id_by_url(app_t *app, db_table_t table, const char *url);

/**
 * @brief Put metadata for a specific table
 * @param app - [in] Pointer to the application instance
 * @param table - [in] Table identifier
 * @param meta - [in] Pointer to the metadata structure
 * @param size - [in] Size of the metadata structure
 * @return True on success, false on failure
 */
bool db_put_meta(app_t *app, db_table_t table, const void *meta, size_t size);

/**
 * @brief Put value by ID
 * @param app - [in] Pointer to the application instance
 * @param table - [in] Table identifier with ID keys
 * @param id - [in] ID key
 * @param value - [in] Pointer to the value data
 * @param size - [in] Size of the value data
 * @return True on success, false on failure
 */
bool db_put_value_by_id(app_t *app, db_table_t table, uint32_t id, const void *value, size_t size);

/**
 * @brief Put ID key by URL
 * @param app - [in] Pointer to the application instance
 * @param url_table - [in] Table identifier with URL keys
 * @param url - [in] URL string
 * @param id_table - [in] Table identifier with ID keys
 * @param id - [in] ID key to associate with the URL
 * @return True on success, false on failure
 */
bool db_put_id_by_url(app_t *app, db_table_t url_table, const char *url, db_table_t id_table, uint32_t id);
