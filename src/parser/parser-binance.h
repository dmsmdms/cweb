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
 * @brief Initialize Binance parser
 * @return PARSER_BIN_ERR_OK on success, error code on failure
 */
parser_bin_err_t parser_bin_init(void);

/**
 * @brief Destroy Binance parser
 */
void parser_bin_destroy(void);
