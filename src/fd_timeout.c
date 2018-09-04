#include "fd_timeout.h"

#include "fd_hal_rtc.h"

void fd_timeout_initialize(fd_timeout_t *timeout, float duration) {
    fd_time_t now = fd_hal_rtc_get_accurate_time();
    timeout->last_time = now;
    timeout->deadline = fd_time_add(now, fd_time_from_float(duration));
    timeout->change_count = 0;
}

bool fd_timeout_is_over(fd_timeout_t *timeout) {
    fd_time_t now = fd_hal_rtc_get_accurate_time();
    if (!fd_time_eq(now, timeout->last_time)) {
        ++timeout->change_count;
        timeout->last_time = now;
    }
    if ((timeout->change_count > 1) && fd_time_gt(now, timeout->deadline)) {
        return true;
    }
    return false;
}
