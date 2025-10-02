#pragma once

#include <common.h>

/**
 * @brief Daemon error codes
 */
typedef enum {
    DAEMON_ERR_OK,       ///< No error
    DAEMON_ERR_FORK,     ///< Fork error
    DAEMON_ERR_PARENT,   ///< Parent process exit
    DAEMON_ERR_CHILD,    ///< Child process continue
    DAEMON_ERR_PID_FILE, ///< PID file write error
    DAEMON_ERR_KILL,     ///< Kill process error
    DAEMON_ERR_MAX,
} daemon_err_t;

/**
 * @brief Make process a daemon
 * @param pid_path - [in] Path to PID file
 * @return DAEMON_ERR_OK on success, error code otherwise
 */
daemon_err_t daemon_init(const char *pid_path);

/**
 * @brief Stop daemon process
 * @param pid_path - [in] Path to PID file
 * @return DAEMON_ERR_OK on success, error code otherwise
 */
daemon_err_t daemon_stop(const char *pid_path);

/**
 * @brief Cleanup daemon resources
 */
void daemon_cleanup(void);
