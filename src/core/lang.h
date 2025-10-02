#pragma once

#include <common.h>

/**
 * @brief Macro to convert two-character language code to integer
 */
#define LANG_CODE(c1, c2) (c1 << 8 | c2)

/**
 * @brief Language error codes enumeration
 */
typedef enum {
    LANG_ERR_OK,     ///< No error
    LANG_ERR_LOAD,   ///< Failed to load language file
    LANG_ERR_PARSE,  ///< Failed to parse language file
    LANG_ERR_NO_MEM, ///< Memory allocation error
    LANG_ERR_MAX,
} lang_err_t;

/**
 * @brief Language codes enumeration
 */
typedef enum {
    LANG_CODE_EN = LANG_CODE('e', 'n'), ///< English
    LANG_CODE_RU = LANG_CODE('r', 'u'), ///< Russian
    LANG_CODE_LT = LANG_CODE('l', 't'), ///< Lithuanian
    LANG_CODE_UK = LANG_CODE('u', 'k'), ///< Ukrainian
    LANG_CODE_MAX,
} lang_code_t;

/**
 * @brief Load language file
 * @param lang_file - [in] Path to the language file
 * @return LANG_ERR_OK on success, error code otherwise
 */
lang_err_t lang_load(const char *lang_file);

/**
 * @brief Free language resources
 */
void lang_free(void);

/**
 * @brief Get string from language file
 * @param lang_code - [in] Language code
 * @param key - [in] Key to look up in the language file
 * @return Pointer to the localized string, or NULL if not found
 */
const char *lang_get_str(lang_code_t lang_code, const char *key);
