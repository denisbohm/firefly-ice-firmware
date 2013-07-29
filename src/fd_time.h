#ifndef FD_TIME_H
#define FD_TIME_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t seconds;
    uint32_t microseconds;
} fd_time_t;

bool fd_time_eq(fd_time_t a, fd_time_t b);
bool fd_time_lt(fd_time_t a, fd_time_t b);
bool fd_time_gt(fd_time_t a, fd_time_t b);

fd_time_t fd_time_add(fd_time_t a, fd_time_t b);
fd_time_t fd_time_subtract(fd_time_t a, fd_time_t b);

#endif