#pragma once

#include <common.h>

#define CSV_GEN_TS(val)                                                                                                \
    {                                                                                                                  \
        csv_gen_ts,                                                                                                    \
        {                                                                                                              \
            .u64val = val                                                                                              \
        }                                                                                                              \
    }
#define CSV_GEN_UINT8(val)                                                                                             \
    {                                                                                                                  \
        csv_gen_uint8,                                                                                                 \
        {                                                                                                              \
            .u8val = val                                                                                               \
        }                                                                                                              \
    }
#define CSV_GEN_FLOAT(val)                                                                                             \
    {                                                                                                                  \
        csv_gen_float,                                                                                                 \
        {                                                                                                              \
            .fval = val                                                                                                \
        }                                                                                                              \
    }

/**
 * @brief CSV generation error codes
 */
typedef enum {
    CSV_GEN_ERR_OK,   ///< No error
    CSV_GEN_ERR_FILE, ///< File error
    CSV_GEN_ERR_EOF,  ///< End of file reached
    CSV_GEN_ERR_DATA, ///< Data error
    CSV_GEN_ERR_MAX,
} csv_gen_err_t;

/**
 * @brief Forward declaration of CSV parse context
 */
typedef struct csv_gen_ctx csv_gen_ctx_t;

/**
 * @brief Union to hold different CSV value types
 */
typedef union {
    uint64_t u64val;
    uint8_t u8val;
    float fval;
} csv_gen_val_t;

/**
 * @brief Callback function type for generating a CSV row
 * @param ctx - [in] CSV generation context
 * @param val - [in] Value to be processed
 * @return CSV_GEN_ERR_OK on success, or an appropriate error code on failure
 */
typedef csv_gen_err_t (*csv_gen_cb_t)(const csv_gen_ctx_t *ctx, csv_gen_val_t val);

/**
 * @brief CSV generation item structure
 */
typedef struct {
    csv_gen_cb_t cb;   ///< Callback function to generate the CSV item
    csv_gen_val_t val; ///< Data associated with the CSV item
} csv_gen_item_t;

/**
 * @brief Callback function type for generating a CSV row
 */
typedef csv_gen_err_t (*csv_row_gen_cb_t)(const csv_gen_ctx_t *ctx, void *priv_data);

/**
 * @brief Generate a CSV file
 * @param file_path - [in] Path to the CSV file
 * @param row_cb - [in] Callback function to generate each row
 * @param names - [in] Comma-separated list of column names
 * @param names_count - [in] Number of column names
 * @param priv_data - [in] Private data for the callback
 * @return CSV_GEN_ERR_OK on success, or an appropriate error code on failure
 */
csv_gen_err_t csv_gen_file(const char *file_path, csv_row_gen_cb_t row_cb, const char *const *names,
                           uint32_t names_count, void *priv_data);

/**
 * @brief Generate a CSV row from items
 * @param ctx - [in] CSV generation context
 * @param items - [in] Array of CSV generation items
 * @param items_count - [in] Number of items in the array
 * @return CSV_GEN_ERR_OK on success, or an appropriate error code on failure
 */
csv_gen_err_t csv_gen(const csv_gen_ctx_t *ctx, const csv_gen_item_t *items, uint32_t items_count);

/**
 * @brief Generate a CSV timestamp value
 * @param ctx - [in] CSV generation context
 * @param val - [in] Value to be processed
 * @return CSV_GEN_ERR_OK on success, or an appropriate error code on failure
 */
csv_gen_err_t csv_gen_ts(const csv_gen_ctx_t *ctx, csv_gen_val_t val);

/**
 * @brief Generate a CSV uint8 value
 * @param ctx - [in] CSV generation context
 * @param val - [in] Value to be processed
 * @return CSV_GEN_ERR_OK on success, or an appropriate error code on failure
 */
csv_gen_err_t csv_gen_uint8(const csv_gen_ctx_t *ctx, csv_gen_val_t val);

/**
 * @brief Generate a CSV float value
 * @param ctx - [in] CSV generation context
 * @param val - [in] Value to be processed
 * @return CSV_GEN_ERR_OK on success, or an appropriate error code on failure
 */
csv_gen_err_t csv_gen_float(const csv_gen_ctx_t *ctx, csv_gen_val_t val);
