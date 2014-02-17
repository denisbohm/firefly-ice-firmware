#include "fd_processor.h"
#include "fd_reset.h"
#include "fd_rtc.h"

#include <em_rmu.h>

#include <string.h>

#define MAGIC 0xa610efcc

uint32_t fd_reset_last_cause;
fd_time_t fd_reset_last_time;

fd_reset_retained_t fd_reset_retained_at_initialize;
static bool retain_was_valid;

static
bool is_ram_retained_reset(uint32_t cause) {
    if (cause == 0) {
        return true;
    }

    if (cause & 1) {
        return false; // Power On Reset
    }
    if (cause & 2) {
        return false; // Brown Out Detector Unregulated Domain Reset
    }
    if (cause & 4) {
        return false; // Brown Out Detector Regulated Domain Reset
    }
    if (cause & 8) {
        return true; // External Pin Reset
    }
    if (cause & 16) {
        return true; // Watchdog Reset
    }
    if (cause & 32) {
        return true; // LOCKUP Reset
    }
    if (cause & 64) {
        return true; // System Request Reset
    }

    return false;
}

void fd_reset_initialize(void) {
    fd_reset_last_cause = RMU_ResetCauseGet();
    RMU_ResetCauseClear();

    fd_reset_retained_t *retained = RETAINED;
    memcpy(&fd_reset_retained_at_initialize, retained, sizeof(fd_reset_retained_t));
    retain_was_valid =
        (retained->magic == MAGIC) &&
        (retained->power_battery_level >= 0.0) && (retained->power_battery_level <= 1.0) &&
        (retained->rtc.seconds > 1381363200) && (retained->rtc.seconds < 2328048000) &&
        (retained->rtc.microseconds < 1000000) &&
        is_ram_retained_reset(fd_reset_last_cause);

    if (!retain_was_valid) {
        memset(retained, 0, sizeof(fd_reset_retained_t));
        retained->magic = MAGIC;
    }

    fd_reset_last_time = retained->rtc;
}

bool fd_reset_retained_was_valid_on_startup(void) {
    return retain_was_valid;
}

void fd_reset_by(uint8_t type) {
    switch (type) {
        case FD_RESET_SYSTEM_REQUEST: {
            NVIC_SystemReset();
        } break;
        case FD_RESET_WATCHDOG: {
            fd_delay_ms(30000);
        } break;
        case FD_RESET_HARD_FAULT: {
            void (*null_fn)(void) = 0;
            (*null_fn)();
        } break;
        case FD_RESET_RETAIN: {
            fd_reset_initialize();
        } break;
        default: {
        } break;
    }
}