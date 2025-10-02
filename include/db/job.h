#pragma once

#include <core/db.h>

/**
 * @brief Job source sites
 */
typedef enum PACKED {
    JOB_SOURCE_NONE,
    JOB_SOURCE_CVBANKAS, ///< Job from cvbankas.lt
    JOB_SOURCE_MAX,
} job_source_t;

/**
 * @brief Salary currency types
 */
typedef enum PACKED {
    JOB_CURRENCY_NONE,
    JOB_CURRENCY_EUR, ///< Euro
    JOB_CURRENCY_MAX,
} job_currency_t;

/**
 * @brief Payment period types
 */
typedef enum PACKED {
    JOB_PAYMENT_NONE,
    JOB_PAYMENT_HOUR,  ///< Hourly
    JOB_PAYMENT_DAY,   ///< Daily
    JOB_PAYMENT_MONTH, ///< Monthly
    JOB_PAYMENT_MAX,
} job_payment_t;

/**
 * @brief Tax types
 */
typedef enum PACKED {
    JOB_TAXES_NONE,
    JOB_TAXES_GROSS, ///< Gross salary
    JOB_TAXES_NET,   ///< Net salary
    JOB_TAXES_MAX,
} job_taxes_t;

/**
 * @brief Key for DB_TABLE_JOB_<region> table and value for DB_TABLE_JOB_<region>_<subkey> table
 */
typedef struct PACKED {
    db_table_t table : 8; ///< Must be either DB_TABLE_JOB_<region>
    uint32_t id : 24;     ///< Autoincrement unique job ID
} db_job_key_t;

/**
 * @brief Subkey for DB_TABLE_JOB_<region>_URL table
 */
typedef struct PACKED {
    db_table_t table;  ///< Must be DB_TABLE_JOB_<region>_URL
    uint8_t url_len;   ///< Length of the url string
    char url[256 - 2]; ///< Job url (null-terminated)
} db_job_key_url_t;

/**
 * @brief Value for DB_TABLE_JOB_<region> table
 */
typedef struct PACKED {
    uint32_t salary_min;         ///< Minimum salary, 1.00 is stored as 100, 0 means not specified
    uint32_t salary_max;         ///< Maximum salary, 1.00 is stored as 100, 0 means not specified
    uint16_t url_offset;         ///< Offset of the URL string in data
    uint16_t city_offset;        ///< Offset of the city string in data
    uint16_t title_offset;       ///< Offset of the job title string in data
    uint16_t description_offset; ///< Offset of the job description string in data
    uint16_t addr_offset;        ///< Offset of the address string in data, 0 means not specified
    job_source_t source;         ///< Job source site
    job_currency_t currency;     ///< Salary currency, JOB_CURRENCY_NONE means not specified
    job_payment_t payment;       ///< Payment period, JOB_PAYMENT_NONE means not specified
    job_taxes_t taxes;           ///< Tax type, JOB_TAXES_NONE means not specified
    bool for_ukraine;            ///< True if the job is available for Ukrainian refugees
    bool for_disabled;           ///< True if the job is available for disabled persons
    char data[8 * 1024 - 24];    ///< Contiguous buffer for all variable length strings
} db_job_value_t;

/**
 * @brief Get job entry by URL
 * @param db - [in] Pointer to the database instance
 * @param url - [in] Job URL
 * @return Pointer to the job entry value, NULL if not found
 */
const db_job_value_t *db_job_get_by_url(const db_t *db, const char *url);
