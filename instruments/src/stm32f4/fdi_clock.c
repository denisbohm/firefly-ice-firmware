#include "fdi_clock.h"

#include <stm32f4xx.h>

void fdi_clock_start_high_speed_internal(void) {
    RCC_HSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET);
    RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);

    FLASH_SetLatency(FLASH_Latency_2);

    RCC_PLLCmd(DISABLE);
    RCC_PLLConfig(RCC_PLLSource_HSI, 16, 336, 4, 7); // SYSCLK 84MHz & USB 48MHz
    RCC_PLLCmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
}
