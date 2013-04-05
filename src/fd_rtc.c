#include "fd_rtc.h"

#include <em_cmu.h>
#include <em_rtc.h>

#include <stdint.h>

static uint32_t rtc_time_seconds;
static uint32_t rtc_time_ticks;

void fd_rtc_initialize(void) {
    rtc_time_seconds = 0;
    rtc_time_ticks = 0;

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

void RTC_IRQHandler(void) {
    RTC_IntClear(RTC_IFC_COMP0);

    if (++rtc_time_ticks >= 32) {
        ++rtc_time_seconds;
        rtc_time_ticks = 0;
    }
}