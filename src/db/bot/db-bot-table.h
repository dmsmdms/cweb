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
 * @brief Database structure to hold bot chat metadata
 */
typedef struct {
    uint32_t chat_count;  ///< Number of chats
    uint32_t last_upd_id; ///< Last update ID processed
} db_bot_chat_meta_t;

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
 * @brief Get bot chat metadata from the database
 * @param meta - [out] Pointer to the bot chat metadata structure to be filled
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_bot_chat_get_meta(db_bot_chat_meta_t *meta);

/**
 * @brief Put bot chat metadata into the database
 * @param meta - [in] Pointer to the bot chat metadata structure to be stored
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_bot_chat_put_meta(const db_bot_chat_meta_t *meta);

/**
 * @brief Put bot chat data into the database
 * @param id - [in] chat ID
 * @param chat - [in] Pointer to the bot chat data to be stored
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_bot_chat_put(uint64_t id, const db_bot_chat_t *chat);

/**
 * @brief Get the next bot chat from the database
 * @param pid - [out] Pointer to the last retrieved chat ID; updated to the current chat ID
 * @param chat - [out] Pointer to the bot chat data structure to be filled
 * @return DB_ERR_OK on success, error code otherwise
 */
db_err_t db_bot_chat_get_next(uint64_t *pid, db_bot_chat_t *chat);
