#pragma once

#include <core/html/html-parser.h>

/**
 * @brief CVBankas parser structure
 */
typedef struct {
    ev_timer update_timer;  ///< Timer for periodic parsing of new job listings
    uint32_t current_page;  ///< Current page being parsed
    uint32_t page_count;    ///< Total number of pages to parse
    uint32_t job_count;     ///< Total number of job listings found
    uint32_t new_jobs;      ///< Number of new jobs found since last parse
    uint32_t parsed_jobs;   ///< Number of new jobs that was parsed
    bool parse_vip_done;    ///< Flag indicating if VIP job listings have been parsed
    bool parse_normal_done; ///< Flag indicating if normal job listings have been parsed
} parser_cvb_t;

/**
 * @brief Initialize CVBankas parser
 * @param app - [in] Pointer to the application instance
 */
void parser_cvb_init(app_t *app);

/**
 * @brief Destroy CVBankas parser
 * @param app - [in] Pointer to the application instance
 */
void parser_cvb_destroy(app_t *app);

/**
 * @brief Parse job information from a HTML page node
 * @param app - [in] Pointer to the application context
 * @param root - [in] Pointer to the root GumboNode of the job HTML page
 * @param url - [in] URL of the job page
 */
void parser_cvb_job_parse(app_t *app, GumboNode *root, const char *url);
