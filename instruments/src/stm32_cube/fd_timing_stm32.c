#include "fd_timing.h"

#include "fdi_stm32.h"

void fd_hal_timing_initialize(void) {
}

bool fd_hal_timing_get_enable(void) {
    return (DWT->CTRL & 0x01) != 0;
}

void fd_hal_timing_enable(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    DWT->CTRL |= 0x01;
    DWT->CYCCNT = 0;
}

void fd_hal_timing_disable(void) {
    DWT->CTRL &= ~0x01;

    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
}

void fd_hal_timing_set_enable(bool enable) {
    if (enable) {
        fd_hal_timing_enable();
    } else {
        fd_hal_timing_disable();
    }
}

void fd_hal_timing_adjust(void) {
}

uint32_t fd_hal_timing_get_timestamp(void) {
    return DWT->CYCCNT;
}
