#include "fd_timeout.h"

#include "fd_nrf5.h"

#define FD_TIMEOUT_TIMER NRF_TIMER4

void fd_timeout_initialize(fd_timeout_t *timeout, float duration) {
    FD_TIMEOUT_TIMER->MODE = TIMER_MODE_MODE_Timer;
    FD_TIMEOUT_TIMER->TASKS_CLEAR = 1;
    FD_TIMEOUT_TIMER->PRESCALER = 4; // prescaler 4 == 1MHz timer clock
    FD_TIMEOUT_TIMER->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
    FD_TIMEOUT_TIMER->EVENTS_COMPARE[0] = 0;
    FD_TIMEOUT_TIMER->CC[0] = (uint32_t)(duration * 1000000);
    FD_TIMEOUT_TIMER->TASKS_START = 1;
}

float fd_timeout_elapsed(fd_timeout_t *timeout) {
    FD_TIMEOUT_TIMER->TASKS_CAPTURE[1] = 1;
    uint32_t count = FD_TIMEOUT_TIMER->CC[1];
    return count / 1000000.0;
}

bool fd_timeout_is_over(fd_timeout_t *timeout) {
    return FD_TIMEOUT_TIMER->EVENTS_COMPARE[0] != 0;
}

void fd_timeout_stop(fd_timeout_t *timeout) {
    FD_TIMEOUT_TIMER->TASKS_STOP = 1;
}