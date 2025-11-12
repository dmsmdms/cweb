#pragma once

#include <core/db/db-table.h>

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
    JOB_PAYMENT_HOURLY,  ///< Hourly
    JOB_PAYMENT_DAYLY,   ///< Daily
    JOB_PAYMENT_MONTHLY, ///< Monthly
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
 * @brief Value for DB_TABLE_JOB and key - db_key_meta_t
 */
typedef struct PACKED {
    uint32_t last_id;   ///< Last used ID in the table
    uint32_t job_count; ///< Total number of jobs in the table
} db_job_meta_t;

/**
 * @brief Value for DB_TABLE_JOB table
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
    char data[16 * 1024 - 24];   ///< Contiguous buffer for all variable length strings
} db_job_t;
_Static_assert(sizeof(db_job_t) == 16 * 1024, "Invalid size of db_job_t");

/**
 * @brief Job entry structure for easier access
 */
typedef struct {
    const char *url;         ///< URL string
    const char *city;        ///< City string, NULL means not specified
    const char *title;       ///< Job title string
    const char *description; ///< Job description string
    const char *addr;        ///< Address string, NULL means not specified
    uint32_t salary_min;     ///< Minimum salary, 1.00 is stored as 100, 0 means not specified
    uint32_t salary_max;     ///< Maximum salary, 1.00 is stored as 100, 0 means not specified
    job_source_t source;     ///< Job source site
    job_currency_t currency; ///< Salary currency, JOB_CURRENCY_NONE means not specified
    job_payment_t payment;   ///< Payment period, JOB_PAYMENT_NONE means not specified
    job_taxes_t taxes;       ///< Tax type, JOB_TAXES_NONE means not specified
    bool for_ukraine;        ///< True if the job is available for Ukrainian refugees
    bool for_disabled;       ///< True if the job is available for disabled persons
} job_t;

/**
 * @brief Get total job count in the database
 * @param app - [in] Pointer to the application structure
 * @return Total number of jobs in the database
 */
uint32_t db_job_get_count(app_t *app);

/**
 * @brief Add job entry to the database
 * @param app - [in] Pointer to the application structure
 * @param job - [in] Pointer to the job entry structure
 * @return true if job was added successfully, false otherwise
 */
bool db_job_add(app_t *app, const job_t *job);

/**
 * @brief Get job entry by ID
 * @param app - [in] Pointer to the application structure
 * @param id - [in] Job ID
 * @param job - [out] Job entry structure to be filled
 * @return true if job was found and job structure filled, false otherwise
 */
// bool db_job_get_by_id(app_t *app, uint32_t id, job_t *job);

/**
 * @brief Get job entry by URL
 * @param app - [in] Pointer to the application structure
 * @param url - [in] Job URL
 * @param job - [out] Job entry structure to be filled, can be NULL to just check existence
 * @return true if job was found and job structure filled, false otherwise
 */
bool db_job_get_by_url(app_t *app, const char *url, job_t *job);
