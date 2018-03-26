#include "fd_delay.h"

#include "fd_nrf5.h"

void fd_delay_ms(uint32_t ms) {
    nrf_delay_ms(ms);
}

void fd_delay_us(uint32_t us) {
    nrf_delay_us(us);
}

void fd_delay_ns(uint32_t ns) {
    nrf_delay_us((ns + 999) / 1000);
}