#include <core/base/str.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void str_buf_init(str_buf_t *buf, char *data, uint32_t size)
{
    buf->data = data;
    buf->size = size;
    buf->offset = 0;
}

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

void buf_putc(str_buf_t *buf, char ch)
{
    if(buf->offset < buf->size) {
        buf->data[buf->offset] = ch;
        buf->offset++;
    }
}

void buf_puts(str_buf_t *buf, const char *str)
{
    uint32_t len = strlen(str);
    if(buf->offset + len >= buf->size) {
        len = buf->size - buf->offset;
    }
    memcpy(buf->data + buf->offset, str, len);
    buf->offset += len;
}

void buf_strtime(str_buf_t *buf, const char *fmt, uint64_t ts)
{
    struct tm tm;
    localtime_r((time_t *)&ts, &tm);
    buf->offset += strftime(buf->data + buf->offset, buf->size - buf->offset, fmt, &tm);
}

void buf_printf(str_buf_t *buf, const char *str, ...)
{
    va_list args;
    va_start(args, str);
    buf->offset += vsnprintf(buf->data + buf->offset, buf->size - buf->offset - 1, str, args);
    va_end(args);
}
