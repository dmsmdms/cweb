#pragma once

#include <common.h>

/**
 * @brief Enumerations of telegram update types
 */
typedef enum {
    TELEBOT_UPDATE_MESSAGE,              ///< Message
    TELEBOT_UPDATE_EDITED_MESSAGE,       ///< Edited message
    TELEBOT_UPDATE_CHANNEL_POST,         ///< Channel post
    TELEBOT_UPDATE_EDITED_CHANNEL_POST,  ///< Edited channel post
    TELEBOT_UPDATE_INLINE_QUERY,         ///< Inline query
    TELEBOT_UPDATE_CHOSEN_INLINE_RESULT, ///< Chosen inline result
    TELEBOT_UPDATE_CALLBACK_QUERY,       ///< Callback query
    TELEBOT_UPDATE_SHIPPING_QUERY,       ///< Shipping query
    TELEBOT_UPDATE_PRE_CHECKOUT_QUERY,   ///< Pre-checkout query
    TELEBOT_UPDATE_POLL,                 ///< Poll
    TELEBOT_UPDATE_POLL_ANSWER,          ///< Poll answer
    TELEBOT_UPDATE_MY_CHAT_MEMBER,       ///< Chat member update
    TELEBOT_UPDATE_TYPE_MAX,
} telebot_update_type_t;

/**
 * @brief Enumerations of telegram chat types
 */
typedef enum {
    TELEBOT_CHAT_PRIVATE,    ///< Private chat
    TELEBOT_CHAT_GROUP,      ///< Group chat
    TELEBOT_CHAT_SUPERGROUP, ///< Supergroup chat
    TELEBOT_CHAT_CHANNEL,    ///< Channel chat
    TELEBOT_CHAT_TYPE_MAX,
} telebot_chat_type_t;

/**
 * @brief Enumerations of telegram message entity types
 */
typedef enum {
    TELEBOT_ENTITY_MENTION,               ///< @username
    TELEBOT_ENTITY_HASHTAG,               ///< #hashtag
    TELEBOT_ENTITY_CASHTAG,               ///< $USD
    TELEBOT_ENTITY_BOT_COMMAND,           ///< /start
    TELEBOT_ENTITY_URL,                   ///< https://example.org
    TELEBOT_ENTITY_EMAIL,                 ///< user@gmail.com
    TELEBOT_ENTITY_PHONE_NUMBER,          ///< +1-212-555-0123
    TELEBOT_ENTITY_BOLD,                  ///< Bold text
    TELEBOT_ENTITY_ITALIC,                ///< Italic text
    TELEBOT_ENTITY_UNDERLINE,             ///< Underlined text
    TELEBOT_ENTITY_STRIKETHROUGH,         ///< Strikethrough text
    TELEBOT_ENTITY_SPOILER,               ///< Spoiler message
    TELEBOT_ENTITY_BLOCKQUOTE,            ///< Blockquote
    TELEBOT_ENTITY_EXPANDABLE_BLOCKQUOTE, ///< Expandable Blockquote
    TELEBOT_ENTITY_CODE,                  ///< Programming code
    TELEBOT_ENTITY_PRE,                   ///< Pre-formatted text
    TELEBOT_ENTITY_TEXT_LINK,             ///< For clickable text URLs
    TELEBOT_ENTITY_TEXT_MENTION,          ///< For users without usernames
    TELEBOT_ENTITY_CUSTOM_EMOJI,          ///< Custom emoji
    TELEBOT_ENTITY_TYPE_MAX,
} telebot_entity_type_t;

/**
 * @brief Enumerations of telegram chat member statuses
 */
typedef enum {
    TELEBOT_CHAT_MEMBER_STATUS_OWNER,      ///< Owner of the chat
    TELEBOT_CHAT_MEMBER_STATUS_ADMIN,      ///< Administrator of the chat
    TELEBOT_CHAT_MEMBER_STATUS_MEMBER,     ///< Normal member of the chat
    TELEBOT_CHAT_MEMBER_STATUS_RESTRICTED, ///< Restricted member of the chat
    TELEBOT_CHAT_MEMBER_STATUS_LEFT,       ///< Left the chat
    TELEBOT_CHAT_MEMBER_STATUS_BANNED,     ///< Banned from the chat
    TELEBOT_CHAT_MEMBER_STATUS_MAX,
} telebot_chat_member_status_t;

/**
 * @brief This object represents a Telegram user or bot
 */
typedef struct {
    uint32_t id;            ///< Unique identifier for this user or bot
    uint16_t language_code; ///< (Optional) IETF language tag of the user's language
    bool is_bot;            ///< True, if this user is a bot
    const char *first_name; ///< User's or bot's first name
    const char *last_name;  ///< (Optional) User's or bot's last name
    const char *username;   ///< (Optional) User's or bot's username
} telebot_user_t;

/**
 * @brief This object represents a chat
 */
typedef struct {
    uint64_t id;              ///< Unique identifier for this chat
    telebot_chat_type_t type; ///< Type of chat
    const char *first_name;   ///< (Optional) First name of the other party in a private chat
    const char *last_name;    ///< (Optional) Last name of the other party in a private chat
    const char *username;     ///< (Optional) Username of the other party in a private chat
} telebot_chat_t;

/**
 * @brief This object represents one special entity in a text message
 */
typedef struct {
    telebot_entity_type_t type; ///< Type of the entity
    uint32_t offset;            ///< Offset in UTF-16 code units to the start of the entity
    uint32_t length;            ///< Length of the entity in UTF-16 code units
} telebot_message_entity_t;

/**
 * @brief Array of message entities
 */
typedef struct {
    telebot_message_entity_t *data; ///< Pointer to array of message entities
    uint32_t count;                 ///< Number of message entities
} telebot_message_entity_arr_t;

/**
 * @brief This object represents a message
 */
typedef struct {
    uint32_t message_id;                   ///< Unique message identifier
    telebot_user_t from;                   ///< Sender of the message
    telebot_chat_t chat;                   ///< Conversation the message belongs to
    uint64_t date;                         ///< Date the message was sent in Unix time
    telebot_message_entity_arr_t entities; ///< (Optional) Special entities
    const char *text;                      ///< (Optional) Text of the message
} telebot_message_t;

/**
 * @brief Union of possible chat member types
 */
typedef struct {
    telebot_chat_member_status_t status; ///< The member's status in the chat
    telebot_user_t user;                 ///< The chat member
    uint64_t until_date;                 ///< Date when restrictions will be lifted for this user in Unix time
} telebot_chat_member_t;

/**
 * @brief This object represents changes in the status of a chat member
 */
typedef struct {
    telebot_user_t from;                   ///< User that changed the chat member status
    telebot_chat_t chat;                   ///< Chat the user belongs to
    uint64_t date;                         ///< Date the change was made in Unix time
    telebot_chat_member_t old_chat_member; ///< Previous information about the chat member
    telebot_chat_member_t new_chat_member; ///< New information about the chat member
} telebot_chat_member_update_t;

/**
 * @brief This object represents an incoming update
 */
typedef struct {
    uint32_t update_id;                ///< The update's unique identifier
    telebot_update_type_t update_type; ///< The type of the update
    union {
        telebot_message_t message;                   ///< New incoming message of any kind — text, photo, sticker, etc
        telebot_chat_member_update_t my_chat_member; ///< Chat member status update
    };
} telebot_update_t;

/**
 * @brief Responce for getUpdates method
 */
typedef struct {
    telebot_update_t *data; ///< List of updates
    uint32_t count;         ///< Updates count
} telebot_update_arr_t;

/**
 * @brief Error structure
 */
typedef struct {
    const char *description; ///< Error description
    uint32_t code;           ///< Error code
} telebot_error_t;

/**
 * @brief Union of all possible responce types
 */
typedef union {
    telebot_update_arr_t updates; ///< Responce for getUpdates method
    telebot_error_t error;        ///< Error responce
} telebot_result_t;

/**
 * @brief General responce structure for all methods
 */
typedef struct {
    telebot_result_t result; ///< Responce data
    bool ok;                 ///< True if the request was successful
} telebot_resp_t;

/**
 * @brief Callback function type for responce handling
 * @param resp - [in] responce data
 */
typedef void (*telebot_resp_cb_t)(const telebot_resp_t *resp);
