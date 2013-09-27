#include "fd_processor.h"
#include "fd_reset.h"
#include "fd_rtc.h"

#include <em_rmu.h>

uint32_t fd_reset_last_cause;
fd_time_t fd_reset_last_time;

void fd_reset_initialize(void) {
    fd_reset_last_cause = RMU_ResetCauseGet();
    RMU_ResetCauseClear();

    fd_reset_last_time = fd_rtc_get_time_retained();
}

void fd_reset_by(uint8_t type) {
    switch (type) {
        case FD_RESET_SYSTEM_REQUEST: {
            NVIC_SystemReset();
        } break;
        case FD_RESET_WATCHDOG: {
            fd_delay_ms(10000);
        } break;
        case FD_RESET_HARD_FAULT: {
            void (*null_fn)(void) = 0;
            (*null_fn)();
        } break;
        default: break;
    }
}