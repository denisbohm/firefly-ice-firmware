#include "fd_hal_processor.h"
#include "fd_hal_reset.h"
#include "fd_hal_rtc.h"

#include <string.h>

#include "apollo2.h"
#include <am_mcu_apollo.h>

#define MAGIC 0xa610efcc

fd_hal_reset_state_t fd_hal_reset_last;

fd_hal_reset_retained_t fd_hal_reset_retained_at_initialize;
static bool retain_was_valid;

fd_hal_reset_retained_t fd_hal_reset_retained_instance;

fd_hal_reset_retained_t *fd_hal_reset_retained(void) {
    return &fd_hal_reset_retained_instance;
}

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

void fd_hal_reset_initialize(void) {
    fd_hal_reset_last.cause = 1;

    fd_hal_reset_retained_t *retained = fd_hal_reset_retained();
    memset(retained, 0, sizeof(fd_hal_reset_retained_t));
    memcpy(&fd_hal_reset_retained_at_initialize, retained, sizeof(fd_hal_reset_retained_t));
    retain_was_valid =
        (retained->magic == MAGIC) &&
        (retained->power_battery_level >= 0.0) && (retained->power_battery_level <= 1.0) &&
        (retained->rtc.seconds > 1381363200) && (retained->rtc.seconds < 2328048000) &&
        (retained->rtc.microseconds < 1000000) &&
        is_ram_retained_reset(fd_hal_reset_last.cause);

    if (!retain_was_valid) {
        memset(retained, 0, sizeof(fd_hal_reset_retained_t));
        retained->magic = MAGIC;
    }

    fd_hal_reset_last.time = retained->rtc;
}

void fd_hal_reset_start_watchdog(void) {
#ifdef DEBUG
#warning debug is defined - watchdog is not enabled
#else
#endif

#ifdef DEBUG_WATCHDOG
#warning watchdog debug is enabled
#endif
}

void fd_hal_reset_feed_watchdog(void) {
}

void fd_hal_reset_push_watchdog_context(const char *context, char *save) {
    const size_t context_size = FD_HAL_RESET_CONTEXT_SIZE;
    size_t length = strlen(context);
    if (length > context_size - 1) {
        length = context_size - 1;
    }
    fd_hal_reset_retained_t *retained = fd_hal_reset_retained();
    char *retained_context = retained->context;
    memcpy(save, retained_context, context_size);
    memset(retained_context, 0, context_size);
    memmove(retained_context, context, length);
}

void fd_hal_reset_pop_watchdog_context(const char* save) {
    fd_hal_reset_retained_t *retained = fd_hal_reset_retained();
    memcpy(retained->context, save, FD_HAL_RESET_CONTEXT_SIZE);
}

bool fd_hal_reset_retained_was_valid_on_startup(void) {
    return retain_was_valid;
}

void fd_hal_reset_by(uint8_t type) {
    switch (type) {
        case FD_HAL_RESET_SYSTEM_REQUEST: {
            NVIC_SystemReset();
        } break;
        case FD_HAL_RESET_WATCHDOG: {
            char save[FD_HAL_RESET_CONTEXT_SIZE];
            fd_hal_reset_push_watchdog_context("rwdt", save);
            fd_hal_processor_delay_ms(120000);
            fd_hal_reset_pop_watchdog_context(save);
        } break;
        case FD_HAL_RESET_HARD_FAULT: {
            void (*null_fn)(void) = 0;
            (*null_fn)();
        } break;
        case FD_HAL_RESET_RETAIN: {
            fd_hal_reset_initialize();
        } break;
        default: {
        } break;
    }
}
