#include "fd_time.h"

fd_time_t fd_time_from_float(float f) {
    uint32_t seconds = (uint32_t)f;
    uint32_t microseconds = (uint32_t)((f - seconds) * 1000000.0f);
    fd_time_t time = {
        .seconds = seconds,
        .microseconds = microseconds
    };
    return time;
}

float fd_time_to_float(fd_time_t time) {
    return time.seconds + time.microseconds / 1000000.0f;
}

fd_time_t fd_time_from_us(uint64_t us) {
    uint32_t seconds = (uint32_t)(us / 1000000);
    uint32_t microseconds = (uint32_t)(us - (seconds * 1000000ULL));
    fd_time_t time = {
        .seconds = seconds,
        .microseconds = microseconds
    };
    return time;
}

uint64_t fd_time_to_us(fd_time_t time) {
    return time.seconds * 1000000ULL + time.microseconds;
}

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
    fd_time_t r = {
        .seconds = (uint32_t)s,
        .microseconds = (uint32_t)us
    };
    return r;
}