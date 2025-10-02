#pragma once

#include <core/base/str.h>

/**
 * @brief File error codes
 */
typedef enum {
    FILE_ERR_OK,        ///< No error
    FILE_ERR_OPEN,      ///< File open error
    FILE_ERR_READ,      ///< File read error
    FILE_ERR_BUF_SMALL, ///< Buffer too small error
    FILE_ERR_MAX,
} file_err_t;

/**
 * @brief Read file content into memory
 * @param path - [in] File path
 * @param content - [out] String to store file content
 * @return FILE_ERR_OK on success, error code otherwise
 */
file_err_t file_read(const char *path, str_t *content);
