#pragma once

#include <core/http/http-server.h>

/**
 * @brief Jobly file request callback
 * @param req - [in] Pointer to HTTP server request structure
 * @param resp - [out] Pointer to HTTP server response structure to be filled
 */
void api_jobly_file_cb(const http_srv_req_t *req, http_srv_resp_t *resp);
