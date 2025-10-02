#pragma once

#include <core/http/http-server.h>

/**
 * @brief Callback of function that generates HTML content
 * @param app - [in] Pointer to the application instance
 * @param buf - [out] Buffer to write HTML content
 * @param argc - [in] Number of arguments
 * @param argv - [in] Array of arguments
 * @return Number of bytes written to buf, or negative value on error
 */
typedef int (*html_gen_cb_t)(app_t *app, char *buf, int argc, char *argv[]);

/**
 * @brief Connect HTML function name with C function
 */
typedef struct {
    const char *name; ///< HTML function name
    html_gen_cb_t cb; ///< Callback of function that generates HTML content
} html_gen_func_t;

/**
 * @brief Generate HTML file based on file from path
 * @param app - [in] Pointer to the application instance
 * @param resp - [out] Pointer to the HTTP response structure
 * @param dir - [in] HTML files directory
 * @param path - [in] HTML file path
 * @param funcs - [in] List of callback functions
 * @param funcs_count - [in] Size of funcs array
 * @return true on success, false on file read error
 */
bool html_gen(app_t *app, http_srv_resp_t *resp, const char *dir, const char *path, const html_gen_func_t *funcs,
              uint32_t funcs_count);
