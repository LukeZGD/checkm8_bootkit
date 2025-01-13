#ifndef LOG_H
#define LOG_H

#include <stdbool.h>

extern bool libbootkit_debug_enabled;

#define debug(fmt, ...) \
    do { \
        if (libbootkit_debug_enabled) { \
            printf(fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#endif
