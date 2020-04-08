#ifndef FD_HAL_RESET_H
#define FD_HAL_RESET_H

#include "fd_time.h"

#include <stdint.h>

#define FD_HAL_RESET_STARTUP_COMMAND_ENTER_STORAGE_MODE 0x4d534543

#ifndef FD_HAL_RESET_CONTEXT_SIZE
#define FD_HAL_RESET_CONTEXT_SIZE 12
#endif

typedef struct {
    uint32_t magic;

    volatile fd_time_t rtc;

    double power_battery_level;

    uint32_t watchdog_pc;
    uint32_t watchdog_lr;
    char watchdog_context[FD_HAL_RESET_CONTEXT_SIZE];

    char context[FD_HAL_RESET_CONTEXT_SIZE];

    uint32_t startup_command;
} fd_hal_reset_retained_t;

typedef struct {
    fd_time_t time;
    uint32_t cause;
    uint32_t pc;
    uint32_t lr;
    char context[FD_HAL_RESET_CONTEXT_SIZE];
} fd_hal_reset_state_t;

#define FD_HAL_RESET_SYSTEM_REQUEST 1
#define FD_HAL_RESET_WATCHDOG 2
#define FD_HAL_RESET_HARD_FAULT 3
#define FD_HAL_RESET_RETAIN 4
#define FD_HAL_RESET_ASSERTION 5

extern fd_hal_reset_state_t fd_hal_reset_last;

void fd_hal_reset_initialize(void);

bool fd_hal_reset_retained_was_valid_on_startup(void);
fd_hal_reset_retained_t *fd_hal_reset_retained(void);
extern fd_hal_reset_retained_t fd_hal_reset_retained_at_initialize;

void fd_hal_reset_by(uint8_t type);

void fd_hal_reset_start_watchdog(void);
void fd_hal_reset_stop_watchdog(void);
void fd_hal_reset_feed_watchdog(void);
void fd_hal_reset_push_watchdog_context(const char *context, char *save);
void fd_hal_reset_pop_watchdog_context(const char *save);

#ifdef FD_RESET_DEBUG_WATCHDOG
#define FD_HAL_RESET_PUSH()\
 char fd_hal_reset_context[FD_HAL_RESET_CONTEXT_SIZE];\
 fd_hal_reset_push_watchdog_context(__FUNCTION__, fd_hal_reset_context)
#define FD_HAL_RESET_POP() fd_hal_reset_pop_watchdog_context(fd_hal_reset_context)
#else
#define FD_HAL_RESET_PUSH()
#define FD_HAL_RESET_POP()
#endif

#endif