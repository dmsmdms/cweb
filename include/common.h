#pragma once

#include <sys/queue.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define PACKED              __attribute__((__packed__))
#define FORMAT_PRINTF(a, b) __attribute__((format(printf, a, b)))
#define ARRAY_SIZE(a)       (sizeof(a) / sizeof(a[0]))

#define BIT_SET(a, b)   a |= (1 << (b))
#define BIT_CLEAR(a, b) a &= ~(1 << (b))

/**
 * @brief Get parent structure from member pointer
 * @param ptr - [in] Pointer to the member
 * @param type - [in] Type of the parent structure
 * @param member - [in] Name of the member within the structure
 * @return Pointer to the parent structure
 */
#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
