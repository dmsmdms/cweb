#pragma once

#include <core/db/db.h>

#define BOT_CHAT_ARR_BUF_SIZE 1024

/**
 * @brief Structure to hold bot user data
 */
typedef struct {
    uint32_t id;            ///< Unique identifier for this user or bot
    uint16_t lang_code;     ///< (Optional) IETF language tag of the user's language
    const char *first_name; ///< User's or bot's first name
    const char *last_name;  ///< (Optional) User's or bot's last name
    const char *username;   ///< (Optional) User's or bot's username
} bot_user_t;

/**
 * @brief Structure to hold bot chat data
 */
typedef struct {
    uint64_t id; ///< Unique identifier for this chat
} bot_chat_t;

/**
 * @brief Structure to hold an array of bot chats
 */
typedef struct {
    bot_chat_t *data; ///< Pointer to array of bot chats
    uint32_t count;   ///< Number of bot chats in the array
} bot_chat_arr_t;

/**
 * @brief Add a bot user to the database
 * @param user - [in] Pointer to the bot user data to be added
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_bot_user_add(const bot_user_t *user);

/**
 * @brief Add a bot chat to the database
 * @param chat - [in] Pointer to the bot chat data to be added
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_bot_chat_add(const bot_chat_t *chat);

/**
 * @brief Get the last update ID from the database
 * @param last_upd_id - [out] Pointer to store the last update ID
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_bot_last_upd_id_get(uint32_t *last_upd_id);

/**
 * @brief Set the last update ID in the database
 * @param last_upd_id - [in] The last update ID to be set
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_bot_last_upd_id_set(uint32_t last_upd_id);

/**
 * @brief Retrieve all bot chats from the database
 * @param arr - [out] Pointer to the structure to hold the array of bot chats
 * @param buf - [in] Buffer for temporary allocations
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_bot_chat_arr_get(bot_chat_arr_t *arr, buf_ext_t *buf);
