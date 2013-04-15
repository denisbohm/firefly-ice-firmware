#ifndef FD_TIME_H
#define FD_TIME_H

#include <stdint.h>

typedef struct {
    uint32_t seconds;
    uint32_t microseconds;
} fd_time_t;

#endif