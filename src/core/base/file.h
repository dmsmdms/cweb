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
    FILE_ERR_PARSE,     ///< Parse callback error
    FILE_ERR_MAX,
} file_err_t;

/**
 * @brief File stream callback type
 * @param data - [in] Data chunk
 * @param len - [in] Length of data chunk
 * @param priv_data - [in] Private data pointer
 * @return FILE_ERR_OK on success, error code otherwise
 */
typedef int32_t (*file_stream_cb_t)(char *data, uint32_t len, void *priv_data);

/**
 * @brief Read file content into memory
 * @param path - [in] File path
 * @param content - [out] String to store file content
 * @return FILE_ERR_OK on success, error code otherwise
 */
file_err_t file_read(const char *path, str_t *content);

/**
 * @brief Stream file content in chunks via callback
 * @param path - [in] File path
 * @param cb - [in] Callback function to handle data chunks
 * @param priv_data - [in] Private data pointer to pass to callback
 * @return FILE_ERR_OK on success, error code otherwise
 */
file_err_t file_stream(const char *path, file_stream_cb_t cb, void *priv_data);
