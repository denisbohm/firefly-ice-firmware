#ifndef FDI_INTERRUPT_H
#define FDI_INTERRUPT_H

#include <stdint.h>

uint32_t fdi_interrupt_disable(void);
void fdi_interrupt_restore(uint32_t primask);

#endif
