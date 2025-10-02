#pragma once

#include <common.h>

/**
 * @brief Log levels of application
 */
typedef enum {
    LOG_LEVEL_ERROR, ///< Show only error messages
    LOG_LEVEL_WARN,  ///< Show warning and error messages
    LOG_LEVEL_INFO,  ///< Show info, warning and error messages
    LOG_LEVEL_DEBUG, ///< Show all messages (debug, info, warning and error)
    LOG_LEVEL_MAX,
} log_level_t;

#define log_error(fmt, ...) log_write(&app->log, LOG_LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...)  log_write(&app->log, LOG_LEVEL_WARN, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)  log_write(&app->log, LOG_LEVEL_INFO, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define log_debug(fmt, ...) log_write(&app->log, LOG_LEVEL_DEBUG, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

/**
 * @brief Instance of the logging handler
 */
typedef struct {
    log_level_t level; ///< Current log level
    int fd;            ///< File descriptor for log output (e.g., STDOUT)
    bool colors_en;    ///< Enable colored output in terminal
} log_t;

/**
 * @brief Initialize the logging handler
 * @param log - [out] Pointer to the logging handler instance
 * @param file - [in] File path for log output (if NULL, use STDOUT)
 * @param level - [in] Log level (error, warning, info, debug)
 * @return true on success, false otherwise
 */
bool log_init(log_t *log, const char *file, log_level_t level);

/**
 * @brief Destroy the logging handler and free resources
 * @param log - [in] Pointer to the logging handler instance
 */
void log_destroy(log_t *log);

/**
 * @brief Log a message with a specific log level
 * @param log - [in] Pointer to the logging handler instance
 * @param lvl - [in] Log level (error, warning, info, debug)
 * @param file - [in] Name of the file where the log message is generated
 * @param line - [in] Line number where the log message is generated
 * @param func - [in] Name of the function where the log message is generated
 * @param fmt - [in] Format string for the log message
 */
void log_write(const log_t *log, log_level_t lvl, const char *file, int line, const char *func, const char *fmt, ...)
        FORMAT_PRINTF(6, 7);
