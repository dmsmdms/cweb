#pragma once

#include <core/base/log.h>

/**
 * @brief Enumeration of argument parsing errors
 */
typedef enum {
    ARGS_ERR_OK,            ///< No error
    ARGS_ERR_INVALID_PARAM, ///< Invalid parameter
    ARGS_ERR_MAX,
} args_err_t;

/**
 * @brief Contains parsed command line arguments
 */
typedef struct {
#ifdef CONFIG_DB
    const char *db_table;       ///< Database table file path (default: NULL, means no table)
    const char *db_prm;         ///< Database import parameter (default: NULL, means no param)
    const char *db_import_file; ///< Database import file path (default: NULL, means no import)
    const char *db_export_file; ///< Database export file path (default: NULL, means no export)
#endif
    const char *cfg_file; ///< Configuration file path (default: NULL, means use built-in config)
    const char *log_file; ///< Log file path (default: NULL, means stdout)
    log_lvl_t log_lvl;    ///< Log level (default: warn)
    bool run_daemon;      ///< Run as daemon flag (default: false)
    bool stop_daemon;     ///< Stop daemon flag (default: false)
} args_t;

/**
 * @brief Parses command line arguments
 * @param args - [out] Parsed arguments
 * @param argc - [in] Arguments count
 * @param argv - [in] Arguments vector
 */
args_err_t args_parse(args_t *args, int argc, char *argv[]);
