#pragma once

#include <common.h>

/**
 * @brief Enumeration of Binance parser error codes
 */
typedef enum {
    PARSER_BIN_ERR_OK,      ///< No error
    PARSER_BIN_ERR_DB,      ///< Database error
    PARSER_BIN_ERR_INVALID, ///< Invalid data
    PARSER_BIN_ERR_NO_MEM,  ///< Not enough memory
    PARSER_BIN_ERR_CONN,    ///< Connection error
    PARSER_BIN_ERR_MAX,
} parser_bin_err_t;

/**
 * @brief Structure to hold Binance parser statistics
 */
typedef struct {
    uint64_t start_ts;    ///< Parser start time
    uint64_t last_upd_ts; ///< Last update time
} parser_bin_stat_t;

/**
 * @brief Initialize Binance parser
 * @return PARSER_BIN_ERR_OK on success, error code on failure
 */
parser_bin_err_t parser_bin_init(void);

/**
 * @brief Get Binance parser statistics
 * @param stat - [out] Pointer to statistics structure
 */
void parser_bin_get_stat(parser_bin_stat_t *stat);

/**
 * @brief Destroy Binance parser
 */
void parser_bin_destroy(void);
