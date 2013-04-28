#include "fd_rtc.h"

#include "fd_processor.h"

#include <em_cmu.h>
#include <em_rtc.h>

#include <stdint.h>

static volatile uint32_t rtc_time_seconds;
static volatile uint32_t rtc_time_microseconds;

void fd_rtc_initialize(void) {
    rtc_time_seconds = 0;
    rtc_time_microseconds = 0;

    CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
    CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
    CMU_ClockEnable(cmuClock_RTC, true);
    CMU_ClockEnable(cmuClock_CORELE, true);

    RTC_CompareSet(0, 1024);
    RTC_CounterReset();
    RTC_IntEnable(RTC_IFC_COMP0);
    NVIC_EnableIRQ(RTC_IRQn);
    RTC_Init_TypeDef init = RTC_INIT_DEFAULT;
    RTC_Init(&init);
}

uint32_t rtc_get_seconds(void) {
    return rtc_time_seconds;
}

fd_time_t rtc_get_time(void) {
    fd_time_t time;
    fd_interrupts_disable();
    time.microseconds = rtc_time_microseconds;
    time.seconds = rtc_time_seconds;
    fd_interrupts_enable();
    return time;
}

fd_time_t rtc_get_accurate_time(void) {
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

    uint32_t microseconds = rtc_time_microseconds + 31250;
    if (microseconds >= 1000000) {
        microseconds -= 1000000;
        ++rtc_time_seconds;
    }
    rtc_time_microseconds = microseconds;
}