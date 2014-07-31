#include "fd_time.h"

bool fd_time_eq(fd_time_t a, fd_time_t b) {
    return (a.seconds == b.seconds) && (a.microseconds == b.microseconds);
}

bool fd_time_lt(fd_time_t a, fd_time_t b) {
    if (a.seconds < b.seconds) {
        return true;
    }
    return (a.seconds == b.seconds) && (a.microseconds < b.microseconds);
}

bool fd_time_gt(fd_time_t a, fd_time_t b) {
    if (a.seconds > b.seconds) {
        return true;
    }
    return (a.seconds == b.seconds) && (a.microseconds > b.microseconds);
}

fd_time_t fd_time_add(fd_time_t a, fd_time_t b) {
    a.microseconds += b.microseconds;
    if (a.microseconds >= 1000000) {
        a.seconds += 1;
        a.microseconds -= 1000000;
    }
    a.seconds += b.seconds;
    return a;
}

fd_time_t fd_time_subtract(fd_time_t a, fd_time_t b) {
    if (fd_time_lt(a, b)) {
        a.microseconds = 0;
        a.seconds = 0;
    } else {
        if (a.microseconds < b.microseconds) {
            a.microseconds += 1000000;
            a.seconds -= 1;
        }
        a.microseconds -= b.microseconds;
        a.seconds -= b.seconds;
    }
    return a;
}

fd_time_t fd_time_multiply(fd_time_t t, unsigned n) {
    uint64_t us = ((uint64_t)t.microseconds) * n;
    uint64_t s = us / 1000000;
    us = us - (s * 1000000);
    fd_time_t r = {.seconds = s, .microseconds = us};
    return r;
}