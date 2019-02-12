#ifndef FD_TIMEOUT_H
#define FD_TIMEOUT_H

#include "fd_time.h"

typedef struct {
    fd_time_t last_time;
    fd_time_t deadline;
    uint32_t change_count;
} fd_timeout_t;

void fd_timeout_initialize(fd_timeout_t *timeout, float duration);
float fd_timeout_elapsed(fd_timeout_t *timeout);
bool fd_timeout_is_over(fd_timeout_t *timeout);
void fd_timeout_stop(fd_timeout_t *timeout);

#endif
