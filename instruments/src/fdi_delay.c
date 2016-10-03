#include "fdi_delay.h"

void fdi_delay_3x_cycles(uint32_t cycles) __attribute__((naked, used));

void fdi_delay_3x_cycles(uint32_t cycles __attribute__((unused))) {
    __asm(
        "    subs r0, #1\n"
        "    bne fdi_delay_3x_cycles\n"
        "    bx lr"
    );
}

#define CYCLES_PER_MICROSECOND 84U

void fdi_delay_ns(uint32_t ns) {
    fdi_delay_3x_cycles(1 + ns * CYCLES_PER_MICROSECOND / (1000U * 3U));
}

void fdi_delay_us(uint32_t us) {
    fdi_delay_3x_cycles((us * CYCLES_PER_MICROSECOND) / 3U);
}

void fdi_delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; ++i) {
        fdi_delay_us(1000);
    }
}