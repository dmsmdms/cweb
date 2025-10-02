#pragma once

#include <core/base/str.h>

#define FILE_PATH_LEN_MAX 1024

/**
 * @brief File error codes
 */
typedef enum {
    FILE_ERR_OK,        ///< No error
    FILE_ERR_OPEN,      ///< File open error
    FILE_ERR_READ,      ///< File read error
    FILE_ERR_WRITE,     ///< File write error
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
 * @brief Read file string content into memory
 * @param path - [in] File path
 * @param content - [out] String to store file content
 * @return FILE_ERR_OK on success, error code otherwise
 */
file_err_t file_read_str(const char *path, str_t *content);

/**
 * @brief Read data from file
 * @param path - [in] File path
 * @param buf - [out] Buffer to store read data
 * @param len - [in] Length of data to read
 * @return FILE_ERR_OK on success, error code otherwise
 */
file_err_t file_read(const char *path, void *buf, uint32_t len);

/**
 * @brief Write data to file
 * @param path - [in] File path
 * @param data - [in] Data to write
 * @param len - [in] Length of data
 * @return FILE_ERR_OK on success, error code otherwise
 */
file_err_t file_write(const char *path, const void *data, uint32_t len);

/**
 * @brief Stream file content in chunks via callback
 * @param path - [in] File path
 * @param cb - [in] Callback function to handle data chunks
 * @param priv_data - [in] Private data pointer to pass to callback
 * @return FILE_ERR_OK on success, error code otherwise
 */
file_err_t file_stream(const char *path, file_stream_cb_t cb, void *priv_data);
