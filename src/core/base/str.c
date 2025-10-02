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

int str_split(char *str, char sep, char **arr, uint32_t arr_size)
{
    uint32_t count = 0;
    while(*str) {
        if(count >= arr_size) {
            return -1; // array too small
        }
        arr[count++] = str;
        str = strchr(str, sep);
        if(str) {
            *str++ = '\0';
        } else {
            break;
        }
    }
    return count;
}

void str_append_cstr(str_t *str, const char *cstr)
{
    uint32_t len = strlen(cstr);
    if(len > str->len) {
        len = str->len;
    }
    memcpy(str->data, cstr, len);
    str->data += len;
    str->len -= len;
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
