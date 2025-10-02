#pragma once

#include <core/base/log.h>

/**
 * @brief Contains parsed command line arguments
 */
typedef struct {
    const char *cfg_file; ///< Configuration file path (default: NULL, means use built-in config)
    const char *log_file; ///< Log file path (default: NULL, means stdout)
    log_lvl_t log_lvl;    ///< Log level (default: warn)
} args_t;

/**
 * @brief Parses command line arguments
 * @param args - [out] Parsed arguments
 * @param argc - [in] Arguments count
 * @param argv - [in] Arguments vector
 */
void args_parse(args_t *args, int argc, char *argv[]);
