#include "fd_spi0.h"

#include <em_usart.h>

void fd_spi0_initialize(void) {
    USART_InitSync_TypeDef spi_setup = USART_INITSYNC_DEFAULT;
    USART_InitSync(USART0, &spi_setup);
}