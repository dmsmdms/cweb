#pragma once

#define _GNU_SOURCE

#include <sys/queue.h>
#include <autoconf.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdarg.h>
#include <search.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <ev.h>

#define BIT_SET(val, nbit)   val |= ((__typeof__(val))1 << (nbit))
#define BIT_CLEAR(val, nbit) val &= ~((__typeof__(val))1 << (nbit))
#define BIT_CHECK(val, nbit) ((val) & ((__typeof__(val))1 << (nbit)))
#define ROUND_DOWN(num, div) ((num) & -(div))
#define ROUND_UP(num, div)   ROUND_DOWN((num) + (div) - 1, div)
#define UNUSED(x)            (void)(x)
#define PACKED               __attribute__((packed))
#define FORMAT_PRINTF(a, b)  __attribute__((format(printf, a, b)))
#define ARRAY_SIZE(a)        (sizeof(a) / sizeof(a[0]))

/**
 * @brief Application structure
 */
typedef struct app app_t;
