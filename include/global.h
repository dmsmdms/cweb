#pragma once

#include <core/parser.h>
#include <core/http.h>
#include <core/log.h>
#include <core/db.h>
#include <db/job.h>
#include <parser/cvbankas.h>

/**
 * @brief Global application state structure
 */
typedef struct {
    struct ev_loop *loop; ///< Event loop
    ev_signal sigint;     ///< SIGINT handler
    ev_signal sigterm;    ///< SIGTERM handler
    log_t log;            ///< Logging handler
    http_t http;          ///< HTTP multi-connection handler
    db_t db;              ///< Database handler
    cvbankas_t cvbankas;  ///< CVBankas parser handler
    bool is_running;      ///< Application running flag
} app_t;
