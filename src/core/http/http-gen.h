#pragma once

#include <core/http/http-server.h>
#include <core/json/json-gen.h>

/**
 * @brief Generate HTTP JSON response
 * @param app - [in] Pointer to application structure
 * @param resp - [out] Pointer to HTTP server response structure to be filled
 * @param code - [in] HTTP response code
 * @param items - [in] Pointer to array of JSON items
 * @param num_items - [in] Number of JSON items
 * @return True on success, false on failure
 */
bool http_gen_resp_json(app_t *app, http_srv_resp_t *resp, http_resp_code_t code, const json_gen_item_t *items,
                        uint32_t num_items);
