#pragma once

#include <db/db-bot.h>

#define BOT_USER_SIZE 512

/**
 * @brief Database structure to hold bot user data
 */
typedef struct {
    uint16_t lang_code;       ///< (Optional) IETF language tag of the user's language
    uint16_t first_name_off;  ///< Offset to the first name string in data
    uint16_t last_name_off;   ///< Offset to the last name string in data
    uint16_t username_off;    ///< Offset to the username string in data
    char data[BOT_USER_SIZE]; ///< Buffer to hold string data
} db_bot_user_t;

/**
 * @brief Database structure to hold bot chat data
 */
typedef struct {
    uint32_t pad; ///< Padding for alignment
} db_bot_chat_t;

/**
 * @brief Put bot user data into the database
 * @param id - [in] user ID
 * @param user - [in] Pointer to the bot user data to be stored
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_bot_user_put(uint32_t id, const db_bot_user_t *user);

/**
 * @brief Put bot chat data into the database
 * @param id - [in] chat ID
 * @param chat - [in] Pointer to the bot chat data to be stored
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_bot_chat_put(uint64_t id, const db_bot_chat_t *chat);
