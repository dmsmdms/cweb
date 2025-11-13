#pragma once

#define _GNU_SOURCE

#include <autoconf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define BIT_SET(val, nbit)   val |= ((__typeof__(val))1 << (nbit))
#define BIT_CLEAR(val, nbit) val &= ~((__typeof__(val))1 << (nbit))
#define BIT_CHECK(val, nbit) ((val) & ((__typeof__(val))1 << (nbit)))

#define ROUND_DOWN(num, div) ((num) & -(div))
#define ROUND_UP(num, div)   ROUND_DOWN((num) + (div) - 1, div)

#define UNUSED              __attribute__((unused))
#define PACKED              __attribute__((packed))
#define CONSTRUCTOR         __attribute__((constructor))
#define FORMAT_PRINTF(a, b) __attribute__((format(printf, a, b)))
#define FORMAT_STRFTIME(a)  __attribute__((format(strftime, a, 0)))
#define ARRAY_SIZE(a)       (sizeof(a) / sizeof(a[0]))
#define STATIC_ASSERT(cond) _Static_assert(cond, #cond)

#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#define return_if_err(func)                                                                                            \
    do {                                                                                                               \
        uint32_t res = func;                                                                                           \
        if(res != 0)                                                                                                   \
            return res;                                                                                                \
    } while(0)

extern bool app_is_running;
