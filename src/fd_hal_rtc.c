#include "fd_event.h"
#include "fd_hal_processor.h"
#include "fd_hal_reset.h"
#include "fd_hal_rtc.h"

#include <em_cmu.h>
#include <em_rtc.h>

#include <stdint.h>

static volatile uint32_t rtc_countdown;

void fd_hal_rtc_initialize(void) {
    rtc_countdown = 0;

    CMU_ClockEnable(cmuClock_CORELE, true);
    CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
    CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
    CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO);
    CMU_ClockEnable(cmuClock_RTC, true);
    CMU_ClockEnable(cmuClock_CORELE, true);
    // output 32kHz clock (for nRF8001)
    CMU->CTRL = (CMU->CTRL & ~_CMU_CTRL_CLKOUTSEL1_MASK) | CMU_CTRL_CLKOUTSEL1_LFXO;
    CMU->ROUTE = CMU_ROUTE_LOCATION_LOC0 | CMU_ROUTE_CLKOUT1PEN;

    fd_hal_rtc_wake();

    RTC_IntEnable(RTC_IFC_COMP0);
    NVIC_EnableIRQ(RTC_IRQn);
    RTC_Init_TypeDef init = RTC_INIT_DEFAULT;
    RTC_Init(&init);
}

void fd_hal_rtc_sleep(void) {
    RTC_CompareSet(0, 65535); // 2 s
}

void fd_hal_rtc_wake(void) {
    RTC_CompareSet(0, 1023); // 31250 us
    RTC_CounterReset();
}

void fd_hal_rtc_set_time(fd_time_t time) {
    fd_hal_processor_interrupts_disable();
    RETAINED->rtc.seconds = time.seconds;
    RETAINED->rtc.microseconds = (time.microseconds / 31250) * 31250;
    fd_hal_processor_interrupts_enable();
}

uint32_t fd_hal_rtc_get_seconds(void) {
    return RETAINED->rtc.seconds;
}

fd_time_t fd_hal_rtc_get_time(void) {
    fd_time_t time;
    fd_hal_processor_interrupts_disable();
    time.microseconds = RETAINED->rtc.microseconds;
    time.seconds = RETAINED->rtc.seconds;
    fd_hal_processor_interrupts_enable();
    return time;
}

fd_time_t fd_hal_rtc_get_accurate_time(void) {
    fd_time_t time;
    uint32_t counter;
    while (true) {
        time.microseconds = RETAINED->rtc.microseconds;
        time.seconds = RETAINED->rtc.seconds;
        counter = RTC_CounterGet();
        uint32_t again = RETAINED->rtc.microseconds;
        if (time.microseconds == again) {
            break;
        }
    }
    time.microseconds += (counter * 31250) / 1024;
    return time;
}

void fd_hal_rtc_set_countdown(uint32_t countdown) {
    fd_hal_processor_interrupts_disable();
    rtc_countdown = countdown;
    fd_hal_processor_interrupts_enable();
}

uint32_t fd_hal_rtc_get_countdown(void) {
    fd_hal_processor_interrupts_disable();
    uint32_t countdown = rtc_countdown;
    fd_hal_processor_interrupts_enable();
    return countdown;
}

void RTC_IRQHandler(void) {
    RTC_IntClear(RTC_IFC_COMP0);

    if (RTC->COMP0 == 1023) {
        uint32_t microseconds = RETAINED->rtc.microseconds + 31250;
        if (microseconds >= 1000000) {
            microseconds -= 1000000;
            ++RETAINED->rtc.seconds;
        }
        RETAINED->rtc.microseconds = microseconds;

        fd_event_set(FD_EVENT_RTC_TICK);

        if (rtc_countdown) {
            if (--rtc_countdown == 0) {
                fd_event_set(FD_EVENT_RTC_COUNTDOWN);
            }
        }
    } else {
        RETAINED->rtc.seconds += 2;
    }
}