#include "fdi_interrupt.h"

#include "fdi_stm32.h"

uint32_t fdi_interrupt_disable(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

void fdi_interrupt_restore(uint32_t primask) {
    __set_PRIMASK(primask);
}
