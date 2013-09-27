#ifndef FD_RESET_H
#define FD_RESET_H

#include "fd_time.h"

#include <stdint.h>

#define FD_RESET_SYSTEM_REQUEST 1
#define FD_RESET_WATCHDOG 2
#define FD_RESET_HARD_FAULT 3

extern uint32_t fd_reset_last_cause;
extern fd_time_t fd_reset_last_time;

void fd_reset_initialize(void);

void fd_reset_by(uint8_t type);

#endif