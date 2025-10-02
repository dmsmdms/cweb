#pragma once

#include <core/base/log.h>

/**
 * @brief Contains parsed command line arguments
 */
typedef struct {
    const char *cfg_file;  ///< Configuration file path (default: NULL, means use built-in config)
    const char *log_file;  ///< Log file path (default: NULL, means stdout)
    log_level_t log_level; ///< Log level (default: warn)
} args_t;

/**
 * @brief Parses command line arguments
 * @param app - [in] Pointer to the application structure
 * @param argc - [in] Arguments count
 * @param argv - [in] Arguments vector
 */
void args_parse(app_t *app, int argc, char *argv[]);
