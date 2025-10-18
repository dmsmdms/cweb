#pragma once

#include <core/base/str.h>

/**
 * @brief File read callback
 * @param app - [in] Pointer to the application instance
 * @param data - [in] File content
 * @param size - [in] File content size
 * @param priv_data - [in] Private data
 * @return true on success, false on failure
 */
typedef bool (*file_read_cb_t)(app_t *app, const char *data, uint32_t size, void *priv_data);

/**
 * @brief File stream callback
 * @param app - [in] Pointer to the application instance
 * @param chunk - [in] File content chunk
 * @param priv_data - [in] Private data
 * @return true on success, false on failure
 */
typedef bool (*file_stream_cb_t)(app_t *app, str_t *chunk, void *priv_data);

/**
 * @brief Read file content and call callback
 * @param app - [in] Pointer to the application instance
 * @param path - [in] File path
 * @param cb - [in] Callback
 * @param priv_data - [in] Private data
 * @return true on success, false on failure
 */
bool file_read(app_t *app, const char *path, file_read_cb_t cb, void *priv_data);

/**
 * @brief Stream file content in chunks and call callback
 * @param app - [in] Pointer to the application instance
 * @param path - [in] File path
 * @param cb - [in] Callback
 * @param priv_data - [in] Private data
 * @return true on success, false on failure
 */
bool file_stream(app_t *app, const char *path, file_stream_cb_t cb, void *priv_data);
