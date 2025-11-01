#pragma once

#include <core/db/db.h>

/**
 * @brief Get value by ID
 * @param table - [in] Name of the database table
 * @param id - [in] ID key
 * @param value - [out] Pointer to the buffer to store the value
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_get_value_by_id(const char *table, uint32_t id, buf_t *value);

/**
 * @brief Put value by ID
 * @param table - [in] Name of the database table
 * @param id - [in] ID key
 * @param value - [in] Pointer to the buffer to store the value
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_put_value_by_id(const char *table, uint32_t id, const buf_t *value);

/**
 * @brief Get ID key by string
 * @param table - [in] Name of the database table
 * @param str - [in] string key
 * @param pid - [out] Pointer to the variable to store the ID key
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_get_id_by_str(const char *table, const char *str, uint32_t *pid);

/**
 * @brief Get next string and its ID key
 * @param table - [in] Name of the database table
 * @param pstr - [out] Pointer to the variable to store the string key
 * @param pstr_len - [out] Pointer to the variable to store the length of the string key
 * @param pid - [out] Pointer to the variable to store the ID key
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_get_str_id_next(const char *table, const char **pstr, uint32_t *pstr_len, uint32_t *pid);

/**
 * @brief Put ID key by string
 * @param table - [in] Name of the database table
 * @param str - [in] string key
 * @param id - [in] ID key to store
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_put_id_by_str(const char *table, const char *str, uint32_t id);

/**
 * @brief Get value by ID and timestamp
 * @param table - [in] Name of the database table
 * @param id - [in] ID key
 * @param ts - [in] Timestamp
 * @param value - [out] Pointer to the buffer to store the value
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_get_value_by_id_ts(const char *table, uint32_t id, uint64_t ts, buf_t *value);

/**
 * @brief Get timestamp and value by ID
 * @param table - [in] Name of the database table
 * @param min_id - [in] ID key
 * @param max_id - [in] Maximum ID key
 * @param min_ts - [in] Minimum timestamp
 * @param max_ts - [in] Maximum timestamp
 * @param pts - [out] Pointer to the variable to store the timestamp
 * @param value - [out] Pointer to the buffer to store the value
 * @param op - [in] Cursor operation
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_get_ts_value_by_id_next(const char *table, uint32_t min_id, uint32_t max_id, uint64_t min_ts,
                                    uint64_t max_ts, uint64_t *pts, buf_t *value, db_cursor_op_t op);

/**
 * @brief Put value by ID and timestamp
 * @param table - [in] Name of the database table
 * @param id - [in] ID key
 * @param ts - [in] Timestamp
 * @param value - [in] Pointer to the buffer to store the value
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_put_value_by_id_ts(const char *table, uint32_t id, uint64_t ts, const buf_t *value);
