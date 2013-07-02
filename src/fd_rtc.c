#include "fd_rtc.h"

#include "fd_processor.h"

#include <em_cmu.h>
#include <em_rtc.h>

#include <stdint.h>

static volatile uint32_t rtc_time_seconds;
static volatile uint32_t rtc_time_microseconds;
static volatile uint32_t rtc_time_tick;

void fd_rtc_initialize(void) {
    rtc_time_seconds = 0;
    rtc_time_microseconds = 0;

    CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
    CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
    CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO);
    CMU_ClockEnable(cmuClock_RTC, true);
    CMU_ClockEnable(cmuClock_CORELE, true);
    // output 32kHz clock (for nRF8001)
    CMU->CTRL = (CMU->CTRL & ~_CMU_CTRL_CLKOUTSEL1_MASK) | CMU_CTRL_CLKOUTSEL1_LFXO;
    CMU->ROUTE = CMU_ROUTE_LOCATION_LOC0 | CMU_ROUTE_CLKOUT1PEN;

    fd_rtc_wake();

    RTC_IntEnable(RTC_IFC_COMP0);
    NVIC_EnableIRQ(RTC_IRQn);
    RTC_Init_TypeDef init = RTC_INIT_DEFAULT;
    RTC_Init(&init);
}

void fd_rtc_sleep(void) {
    rtc_time_tick = 1000000;
    RTC_CompareSet(0, 65536);
}

void fd_rtc_wake(void) {
    rtc_time_tick = 31250;
    RTC_CompareSet(0, 1024);
    RTC_CounterReset();
}

uint32_t fd_rtc_get_seconds(void) {
    return rtc_time_seconds;
}

fd_time_t fd_rtc_get_time(void) {
    fd_time_t time;
    fd_interrupts_disable();
    time.microseconds = rtc_time_microseconds;
    time.seconds = rtc_time_seconds;
    fd_interrupts_enable();
    return time;
}

fd_time_t fd_rtc_get_accurate_time(void) {
    fd_time_t time;
    uint32_t counter;
    while (true) {
        time.microseconds = rtc_time_microseconds;
        time.seconds = rtc_time_seconds;
        counter = RTC_CounterGet();
        uint32_t again = rtc_time_microseconds;
        if (time.microseconds == again) {
            break;
        }
    }
    time.microseconds += (counter * 31250) / 1024;
    return time;
}

void RTC_IRQHandler(void) {
    RTC_IntClear(RTC_IFC_COMP0);

    if (RTC->COMP0 == 65536) {
        uint32_t microseconds = rtc_time_microseconds + 31250;
        if (microseconds >= 1000000) {
            microseconds -= 1000000;
            ++rtc_time_seconds;
        }
        rtc_time_microseconds = microseconds;
    } else {
        rtc_time_seconds += 2;
    }
}