#include "fd_spi1.h"

#include <em_usart.h>

void fd_spi1_initialize(void) {
    USART_InitSync_TypeDef spi_setup = USART_INITSYNC_DEFAULT;
    USART_InitSync(USART1, &spi_setup);
}