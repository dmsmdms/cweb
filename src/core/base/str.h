#pragma once

#include <common.h>

/**
 * @brief Macro to create a str_t from a string literal
 */
#define STR(s) { s, sizeof(s) - 1 }

/**
 * @brief String structure with data pointer and length
 */
typedef struct {
    char *data; ///< Pointer to the string data
    size_t len; ///< Length of the string
} str_t;

/**
 * @brief String buffer structure with data pointer, size, and offset
 */
typedef struct {
    char *data;      ///< Pointer to the string buffer
    uint32_t size;   ///< Total size of the buffer
    uint32_t offset; ///< Current offset in the buffer
} str_buf_t;

/**
 * @brief Initialize a string buffer
 * @param buf - [out] The string buffer to initialize
 * @param data - [in] The data pointer for the buffer
 * @param size - [in] The size of the buffer
 */
void str_buf_init(str_buf_t *buf, char *data, uint32_t size);

/**
 * @brief Remove all occurrences of a character from a string
 * @param str - [in,out] The string to modify
 * @param ch - [in] The character to remove
 */
void str_remove_char(char *str, char ch);

/**
 * @brief Replace all occurrences of a character in a string with another character
 * @param str - [in,out] The string to modify
 * @param old_ch - [in] The character to replace
 * @param new_ch - [in] The character to replace with
 */
void str_replace_char(char *str, char old_ch, char new_ch);

/**
 * @brief Split a string by a separator into an array of strings
 * @param str - [in] The string to split
 * @param sep - [in] The separator character
 * @param arr - [out] The array to store the split strings
 * @param arr_size - [in] The size of the array
 * @return The number of strings stored in the array, -1 if the array is too small
 */
int str_split(char *str, char sep, char *arr[], uint32_t arr_size);

/**
 * @brief Check if one of the strings in an array exists in the given string
 * @param str - [in] The string to check
 * @param arr - [in] The array of strings to compare against
 * @param arr_size - [in] The size of the array
 * @return true if any string from the array is found in the given string, false otherwise
 */
bool str_in_arr(const char *str, const char *arr[], uint32_t arr_size);

/**
 * @brief Remove the first occurrence of any string from an array in the given string
 * @param str - [in,out] The string to modify
 * @param arr - [in] The array of strings to remove
 * @param arr_size - [in] The size of the array
 * @return true if any string was removed, false otherwise
 */
bool str_remove_str_arr(char *str, const char *arr[], uint32_t arr_size);

/**
 * @brief Append a character to a string buffer
 * @param buf - [in,out] The string buffer to write into
 * @param ch - [in] The character to append
 */
void buf_putc(str_buf_t *buf, char ch);

/**
 * @brief Append a string to a string buffer
 * @param buf - [in,out] The string buffer to write into
 * @param str - [in] The string to append
 */
void buf_puts(str_buf_t *buf, const char *str);

/**
 * @brief Append a formatted time string to a string buffer
 * @param buf - [in,out] The string buffer to write into
 * @param fmt - [in] The time format string
 * @param ts - [in] The timestamp to format
 */
void buf_strtime(str_buf_t *buf, const char *fmt, uint64_t ts) FORMAT_STRFTIME(2);

/**
 * @brief Print formatted string into a string buffer
 * @param buf - [in,out] The string buffer to write into
 * @param str - [in] The format string
 * @param ... - [in] The format arguments
 */
void buf_printf(str_buf_t *buf, const char *str, ...) FORMAT_PRINTF(2, 3);
