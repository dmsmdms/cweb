#include <global.h>

void str_remove_char(char *str, char ch)
{
    char *dst = str;
    while(*str) {
        if(*str != ch) {
            *dst++ = *str;
        }
        str++;
    }
    *dst = '\0';
}

void str_replace_char(char *str, char old_ch, char new_ch)
{
    while(*str) {
        if(*str == old_ch) {
            *str = new_ch;
        }
        str++;
    }
}

bool str_in_arr(const char *str, const char *arr[], uint32_t arr_size)
{
    for(uint32_t i = 0; i < arr_size; i++) {
        if(strstr(str, arr[i])) {
            return true;
        }
    }
    return false;
}

bool str_remove_str_arr(char *str, const char *arr[], uint32_t arr_size)
{
    for(uint32_t i = 0; i < arr_size; i++) {
        const char *sub = arr[i];
        char *pos = strstr(str, sub);
        if(pos) {
            char *left = pos + strlen(sub);
            memmove(pos, left, strlen(left) + 1);
            return true;
        }
    }
    return false;
}
