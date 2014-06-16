#ifndef FD_RESET_H
#define FD_RESET_H

#include "fd_time.h"

#include <stdint.h>

#define FD_RESET_RETAINED_BASE 0x20000000

typedef struct {
    uint32_t magic;

    volatile fd_time_t rtc;

    double power_battery_level;

    uint32_t watchdog_lr;
    uint32_t watchdog_pc;
    uint8_t watchdog_context[4];

    uint8_t context[4];
} fd_reset_retained_t;

#define RETAINED ((fd_reset_retained_t *)FD_RESET_RETAINED_BASE)

#define FD_RESET_SYSTEM_REQUEST 1
#define FD_RESET_WATCHDOG 2
#define FD_RESET_HARD_FAULT 3
#define FD_RESET_RETAIN 4

extern uint32_t fd_reset_last_cause;
extern fd_time_t fd_reset_last_time;

void fd_reset_initialize(void);

bool fd_reset_retained_was_valid_on_startup(void);
extern fd_reset_retained_t fd_reset_retained_at_initialize;

void fd_reset_by(uint8_t type);

void fd_reset_start_watchdog(void);
void fd_reset_feed_watchdog(void);
void fd_reset_push_watchdog_context(char *context);
void fd_reset_pop_watchdog_context(void);

#endif