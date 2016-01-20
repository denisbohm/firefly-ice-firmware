#include "fd_hal_processor.h"
#include "fd_hal_reset.h"
#include "fd_hal_rtc.h"

#include <em_cmu.h>
#include <em_rmu.h>
#include <em_timer.h>
#include <em_wdog.h>

#include <string.h>

#define MAGIC 0xa610efcc

uint32_t fd_hal_reset_last_cause;
fd_time_t fd_hal_reset_last_time;

fd_hal_reset_retained_t fd_hal_reset_retained_at_initialize;
static bool retain_was_valid;

#define FD_HAL_RESET_RETAINED_BASE 0x20000000

fd_hal_reset_retained_t *fd_hal_reset_retained(void) {
    return (fd_hal_reset_retained_t *)FD_HAL_RESET_RETAINED_BASE;
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
    fd_hal_reset_last_cause = RMU_ResetCauseGet();
    RMU_ResetCauseClear();

    fd_hal_reset_retained_t *retained = RETAINED;
    memcpy(&fd_hal_reset_retained_at_initialize, retained, sizeof(fd_hal_reset_retained_t));
    retain_was_valid =
        (retained->magic == MAGIC) &&
        (retained->power_battery_level >= 0.0) && (retained->power_battery_level <= 1.0) &&
        (retained->rtc.seconds > 1381363200) && (retained->rtc.seconds < 2328048000) &&
        (retained->rtc.microseconds < 1000000) &&
        is_ram_retained_reset(fd_hal_reset_last_cause);

    if (!retain_was_valid) {
        memset(retained, 0, sizeof(fd_hal_reset_retained_t));
        retained->magic = MAGIC;
    }

    fd_hal_reset_last_time = retained->rtc;
}

void fd_hal_reset_start_watchdog(void) {
#ifdef DEBUG
#warning debug is defined - watchdog is not enabled
#else
    CMU_ClockEnable(cmuClock_CORELE, true);
    WDOG_Init_TypeDef wdog_init = WDOG_INIT_DEFAULT;
    wdog_init.perSel = wdogPeriod_16k;
//    wdog_init.lock = true;
    WDOG_Init(&wdog_init);
#endif

#ifdef DEBUG_WATCHDOG
#warning watchdog debug is enabled
    CMU_ClockEnable(cmuClock_TIMER2, true);
    TIMER_CounterSet(TIMER2, 0);
    TIMER_TopSet(TIMER2, 0xffff);
    TIMER_Init_TypeDef init = TIMER_INIT_DEFAULT;
    init.debugRun = false;
    init.prescale = timerPrescale1024;
    init.clkSel = timerClkSelHFPerClk;
    TIMER_Init(TIMER2, &init);
    TIMER_IntEnable(TIMER2, TIMER_IF_OF);
    NVIC_EnableIRQ(TIMER2_IRQn);
#endif
}

void fd_hal_reset_feed_watchdog(void) {
    WDOG_Feed();

#ifdef DEBUG_WATCHDOG
    TIMER_CounterSet(TIMER2, 0);
#endif
}

void fd_hal_reset_push_watchdog_context(char *context) {
    int length = strlen(context);
    if (length > 4) {
        length = 4;
    }
    fd_hal_reset_retained_t *retained = RETAINED;
    memset(retained->context, 0, 4);
    memcpy(retained->context, context, length);
}

void fd_hal_reset_pop_watchdog_context(void) {
    fd_hal_reset_retained_t *retained = RETAINED;
    memset(retained->context, 0, 4);
}

#ifdef DEBUG_WATCHDOG
void TIMER2_IRQHandler(void) {
    // record call stack info...  before watchdog resets...
    __asm volatile(
        " movw r0, #0x0818\n" // TIMER2->IFC = TIMER_IF_OF;
        " movt r0, #0x4001\n"
        " mov r1, #1\n"
        " str r1, [r0]\n"

        " mrs r0, msp\n" // r0 = msp

        " movw r1, #0x0000\n" // r1 = retained
        " movt r1, #0x2000\n"

        " ldr r2, [r0, #24]\n" // r2 = msp[6]
        " str r2, [r1, #20]\n" // retained->watchdog_lr = r2

        " ldr r2, [r0, #28]\n" // r2 = msp[7]
        " str r2, [r1, #24]\n" // retained->watchdog_pc = r2

        " ldr r2, [r1, #36]\n" // r2 = retained->context
        " str r2, [r1, #28]\n" // retained->watchdog_context = r2
    );
}
#endif

bool fd_hal_reset_retained_was_valid_on_startup(void) {
    return retain_was_valid;
}

void fd_hal_reset_by(uint8_t type) {
    switch (type) {
        case FD_HAL_RESET_SYSTEM_REQUEST: {
            NVIC_SystemReset();
        } break;
        case FD_HAL_RESET_WATCHDOG: {
            fd_hal_reset_push_watchdog_context("rwdt");
            fd_hal_processor_delay_ms(120000);
            fd_hal_reset_pop_watchdog_context();
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
