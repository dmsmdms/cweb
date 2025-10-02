#pragma once

#include <core/telebot/telebot-types.h>

/**
 * @brief Responce for getUpdates method
 */
typedef struct {
    telebot_update_t *data; ///< List of updates
    uint32_t count;         ///< Updates count
} telebot_update_arr_t;

/**
 * @brief Union of all possible responce types
 */
typedef union {
    telebot_update_arr_t updates; ///< Responce for getUpdates method
} telebot_result_t;

/**
 * @brief General responce structure for all methods
 */
typedef struct {
    app_t *app;              ///< Pointer to the application structure
    telebot_result_t result; ///< Responce data
    bool ok;                 ///< True if the request was successful
} telebot_resp_t;

/**
 * @brief Callback function type for responce handling
 * @param resp - [in] responce data
 */
typedef void (*telebot_resp_cb_t)(const telebot_resp_t *resp);

/**
 * @brief Private data for POST request
 */
typedef struct {
    telebot_resp_cb_t cb;
} telebot_post_data_t;
