#include "fd_delay.h"

void fd_delay_3x_cycles(uint32_t cycles) __attribute__((naked, used));

void fd_delay_3x_cycles(uint32_t cycles __attribute__((unused))) {
    __asm(
        "    subs r0, #1\n"
        "    bne fd_delay_3x_cycles\n"
        "    bx lr"
    );
}

#define CYCLES_PER_MICROSECOND 84U

void fd_delay_ns(uint32_t ns) {
    fd_delay_3x_cycles(1 + ns * CYCLES_PER_MICROSECOND / (1000U * 3U));
}

void fd_delay_us(uint32_t us) {
    fd_delay_3x_cycles((us * CYCLES_PER_MICROSECOND) / 3U);
}

void fd_delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; ++i) {
        fd_delay_us(1000);
    }
}
