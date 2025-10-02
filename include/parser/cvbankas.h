#pragma once

#include <core/ev.h>

/**
 * @brief CVBankas parser structure
 */
typedef struct {
    ev_timer_t update_timer; ///< Timer for periodic parsing of new job listings
    uint32_t current_page;   ///< Current page being parsed
    uint32_t page_count;     ///< Total number of pages to parse
    uint32_t job_count;      ///< Total number of job listings found
    bool parse_vip_done;     ///< Flag indicating if VIP job listings have been parsed
    bool parse_normal_done;  ///< Flag indicating if normal job listings have been parsed
} cvbankas_t;

/**
 * @brief Initialize CVBankas parser
 * @param parser - [out] Pointer to CVBankas parser structure
 */
void cvbankas_init(cvbankas_t *parser);

/**
 * @brief Destroy CVBankas parser
 * @param parser - [in] Pointer to CVBankas parser structure
 */
void cvbankas_destroy(cvbankas_t *parser);
