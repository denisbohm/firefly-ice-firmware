#include "fd_timing.h"

#include <am_mcu_apollo.h>

void fd_hal_timing_start(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    DWT->CTRL |= 0x01;
    DWT->CYCCNT = 0;
}

void fd_hal_timing_adjust(void) {
}

bool fd_hal_timing_get_enable(void) {
    return DWT->CTRL & 0x01;
}

void fd_hal_timing_end(void) {
    DWT->CTRL &= (uint32_t)~0x01;

    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
}

uint32_t fd_hal_timing_get_timestamp(void) {
    return DWT->CYCCNT; // could divide by 48 to get uS...
}

void fd_hal_timing_set_enable(bool enable) {
    if (enable) {
        fd_hal_timing_start();
    } else {
        fd_hal_timing_end();
    }
}

void fd_hal_timing_initialize(void) {
    fd_hal_timing_end();
}

