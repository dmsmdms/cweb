#pragma once

#include <calc/calc-crypto.h>

/**
 * @brief Initialize calculation context for a given cryptocurrency symbol
 * @param sym_name - [in] Name of the cryptocurrency symbol
 * @param psym_id - [out] Pointer to store the symbol ID
 * @param calc - [out] Pointer to the calculation context to be initialized
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_crypto_init_calc(const char *sym_name, uint32_t *psym_id, calc_crypto_ctx_t *calc);
