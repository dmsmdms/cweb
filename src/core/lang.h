#pragma once

#include <common.h>

/**
 * @brief Language codes enumeration
 */
typedef enum {
    LANG_EN, ///< English
    LANG_RU, ///< Russian
    LANG_LT, ///< Lithuanian
    LANG_UK, ///< Ukrainian
    LANG_MAX,
} lang_code_t;

/**
 * @brief Language asset structure
 */
typedef struct {
    const char *str[LANG_MAX]; ///< Array of strings for each language
} lang_asset_t;

/**
 * @brief Language structure
 */
typedef struct {
    const char *data;      ///< Raw language data
    lang_asset_t *assets;  ///< Array of language assets
    uint32_t data_size;    ///< Size of the raw language data
    uint32_t assets_count; ///< Number of language assets
} lang_t;

/**
 * @brief Load language file
 * @param app - [in] Pointer to the application instance
 * @return true on success, false on failure
 */
bool lang_load(app_t *app);

/**
 * @brief Free language resources
 * @param app - [in] Pointer to the application instance
 */
void lang_free(app_t *app);

/**
 * @brief Get string from language file
 * @param app - [in] Pointer to the application instance
 * @param lang_code - [in] Language code (e.g., "en", "fr")
 * @param key - [in] Key to look up in the language file
 * @return Pointer to the localized string, or NULL if not found
 */
const char *lang_get_str(app_t *app, lang_code_t lang_code, const char *key);

/**
 * @brief HTML language callback
 * @param app - [in] Pointer to the application instance
 * @param buf - [out] Buffer to store the resulting string
 * @param lang_code - [in] Language code (e.g., "en", "fr")
 * @param argc - [in] Number of arguments
 * @param argv - [in] Array of argument strings
 * @return Length of the resulting string written to buf
 */
int lang_html_cb(app_t *app, char *buf, lang_code_t lang_code, int argc, char **argv);
