#pragma once

#include <core/db/db.h>

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
