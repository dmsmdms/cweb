#pragma once

#include <common.h>

#define STR(s) { s, sizeof(s) - 1 }

/**
 * @brief String structure with data pointer and length
 */
typedef struct {
    char *data; ///< Pointer to the string data
    size_t len; ///< Length of the string
} str_t;

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
