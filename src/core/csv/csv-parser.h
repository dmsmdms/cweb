#pragma once

#include <common.h>

/**
 * @brief CSV parsing error codes
 */
typedef enum {
    CSV_PARSE_ERR_OK,      ///< No error
    CSV_PARSE_ERR_FILE,    ///< Error opening file
    CSV_PARSE_ERR_INVALID, ///< Invalid CSV
    CSV_PARSE_ERR_DECODE,  ///< Error decoding value
    CSV_PARSE_ERR_ABORT,   ///< Parsing aborted
    CSV_PARSE_MAX,
} csv_parse_err_t;

/**
 * @brief Callback function type for parsing a CSV column
 * @param ctx - [in] CSV parse context
 * @param col - [in] Column string
 * @param priv_data - [out] Private data for the callback
 * @return CSV_PARSE_ERR_OK on success, or an appropriate error code on failure
 */
typedef csv_parse_err_t (*csv_parse_cb_t)(const char *col, void *priv_data);

/**
 * @brief CSV item definition
 */
typedef struct {
    csv_parse_cb_t cb; ///< Callback function for parsing the column
    void *priv_data;   ///< Private data for the callback
} csv_item_t;

/**
 * @brief Forward declaration of CSV parse context
 */
typedef struct csv_parse_ctx csv_parse_ctx_t;

/**
 * @brief Callback function type for parsing a CSV row
 * @param ctx - [in] CSV parse context
 * @param cols - [in] Array of column strings in the row
 * @param cols_count - [in] Number of columns in the row
 * @param priv_data - [in] Private data for the callback
 * @return CSV_PARSE_ERR_OK on success, or an appropriate error code on failure
 */
typedef csv_parse_err_t (*csv_row_cb_t)(const csv_parse_ctx_t *ctx, const char **cols, const uint32_t cols_count,
                                        void *priv_data);

/**
 * @brief Parse a CSV file
 * @param file_path - [in] Path to the CSV file
 * @param row_cb - [in] Callback function to handle each parsed row
 * @param names - [in] Comma-separated list of column names to parse
 * @param names_count - [in] Number of column names
 * @param priv_data - [in] Private data for the callback
 * @return CSV_PARSE_ERR_OK on success, or an appropriate error code on failure
 */
csv_parse_err_t csv_parse_file(const char *file_path, csv_row_cb_t row_cb, const char *const *names,
                               uint32_t names_count, void *priv_data);

/**
 * @brief Parse a CSV row
 * @param ctx - [in] CSV parse context
 * @param cols - [in] Array of column strings in the row
 * @param cols_count - [in] Number of columns in the row
 * @param items - [in] Array of CSV items defining how to parse each column
 * @param items_count - [in] Number of CSV items
 * @return CSV_PARSE_ERR_OK on success, or an appropriate error code on failure
 */
csv_parse_err_t csv_parse(const csv_parse_ctx_t *ctx, const char **cols, const uint32_t cols_count,
                          const csv_item_t *items, uint32_t items_count);

/**
 * @brief Parse a timestamp from a CSV column
 * @param ctx - [in] CSV parse context
 * @param col - [in] Column string containing the timestamp
 * @param priv_data - [out] Private data for the callback
 * @return CSV_PARSE_ERR_OK on success, or an appropriate error code on failure
 */
csv_parse_err_t csv_parse_ts(const char *col, void *priv_data);

/**
 * @brief Parse a float from a CSV column
 * @param ctx - [in] CSV parse context
 * @param col - [in] Column string containing the float
 * @param priv_data - [out] Private data for the callback
 * @return CSV_PARSE_ERR_OK on success, or an appropriate error code on failure
 */
csv_parse_err_t csv_parse_float(const char *col, void *priv_data);

/**
 * @brief Parse an 8bit integer from a CSV column
 * @param ctx - [in] CSV parse context
 * @param col - [in] Column string containing the integer
 * @param priv_data - [out] Private data for the callback
 * @return CSV_PARSE_ERR_OK on success, or an appropriate error code on failure
 */
csv_parse_err_t csv_parse_uint8(const char *col, void *priv_data);
