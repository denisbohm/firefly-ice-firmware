#include "fd_delay.h"

void fd_delay_3x_cycles(uint32_t cycles) __attribute__((naked, used));

void fd_delay_3x_cycles(uint32_t cycles __attribute__((unused))) {
    __asm(
        "    subs r0, #1\n"
        "    bne fd_delay_3x_cycles\n"
        "    bx lr"
    );
}

#define CYCLES_PER_SECOND 48000000

void fd_delay_ns(uint32_t ns) {
    fd_delay_3x_cycles((ns * CYCLES_PER_SECOND) / 3000000000);
}

void fd_delay_us(uint32_t us) {
    fd_delay_3x_cycles((us * CYCLES_PER_SECOND) / 3000000);
}

void fd_delay_ms(uint32_t ms) {
    while (ms--) {
        fd_delay_3x_cycles(CYCLES_PER_SECOND / 3000);
    }
}
