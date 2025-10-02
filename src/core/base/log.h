#pragma once

#include <common.h>
#include <sys/queue.h>

/**
 * @brief Log levels of application
 */
typedef enum {
    LOG_LVL_ERROR, ///< Show only error messages
    LOG_LVL_WARN,  ///< Show warning and error messages
    LOG_LVL_INFO,  ///< Show info, warning and error messages
    LOG_LVL_DEBUG, ///< Show all messages (debug, info, warning and error)
    LOG_LVL_MAX,
    LOG_LVL_DEFAULT = LOG_LVL_MAX, ///< Default log level for all files
} log_lvl_t;

/**
 * @brief Log error codes
 */
typedef enum {
    LOG_ERR_OK,        ///< No error
    LOG_ERR_FILE_OPEN, ///< Failed to open log file
    LOG_ERR_NOT_FOUND, ///< Log module not found
    LOG_ERR_SYS,       ///< System error
    LOG_ERR_MAX,
} log_err_t;

/**
 * @brief Log structure for C file module
 */
typedef struct log_mod {
    SLIST_ENTRY(log_mod) entry; ///< Linked list entries
    const char *fname;          ///< File name of current C file
    log_lvl_t lvl;              ///< Log level of current C file
} log_mod_t;

/**
 * @brief Macro to define and initialize log module for current C file
 * @param lvl - [in] Log level for current C file
 */
#define LOG_MOD_INIT(LVL)                                                                                              \
    static log_mod_t log_mod = {                                                                                       \
        .fname = __FILE__,                                                                                             \
        .lvl = LVL,                                                                                                    \
    };                                                                                                                 \
    CONSTRUCTOR static void log_mod_init_wrap(void)                                                                    \
    {                                                                                                                  \
        log_mod_init(&log_mod);                                                                                        \
    }

#define log_error(fmt, ...) log_write(&log_mod, LOG_LVL_ERROR, __LINE__, __func__, fmt __VA_OPT__(, ) __VA_ARGS__)
#define log_warn(fmt, ...)  log_write(&log_mod, LOG_LVL_WARN, __LINE__, __func__, fmt __VA_OPT__(, ) __VA_ARGS__)
#define log_info(fmt, ...)  log_write(&log_mod, LOG_LVL_INFO, __LINE__, __func__, fmt __VA_OPT__(, ) __VA_ARGS__)
#define log_debug(fmt, ...) log_write(&log_mod, LOG_LVL_DEBUG, __LINE__, __func__, fmt __VA_OPT__(, ) __VA_ARGS__)

/**
 * @brief Initialize the log module
 * @param mod - [in,out] Log module to initialize
 */
void log_mod_init(log_mod_t *mod);

/**
 * @brief Set log level for specific C file module
 * @param fname - [in] File name of the C file module
 * @param lvl - [in] Log level to set
 * @return LOG_ERR_OK on success, error code otherwise
 */
log_err_t log_mod_set_lvl(const char *fname, log_lvl_t lvl);

/**
 * @brief Initialize the logging handler
 * @param lvl - [in] Default log level
 * @param log_file - [in] Path to the log file, NULL for stdout
 * @return LOG_ERR_OK on success, error code otherwise
 */
log_err_t log_init(log_lvl_t lvl, const char *log_file);

/**
 * @brief Destroy the logging handler and free resources
 */
void log_destroy(void);

/**
 * @brief Log a message with a specific log level
 * @param mod - [in] Log module where the log message is generated
 * @param lvl - [in] Log level (error, warning, info, debug)
 * @param line - [in] Line number where the log message is generated
 * @param func - [in] Name of the function where the log message is generated
 * @param fmt - [in] Format string for the log message
 */
void log_write(const log_mod_t *mod, log_lvl_t lvl, uint32_t line, const char *func, const char *fmt, ...)
        FORMAT_PRINTF(5, 6);
