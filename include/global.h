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
    log_t log;            ///< Logging handler
    http_t http;          ///< HTTP multi-connection handler
    db_t db;              ///< Database handler
    cvbankas_t cvbankas;  ///< CVBankas parser handler
} app_t;
