#ifndef FDI_GPIO_H
#define FDI_GPIO_H

#include <stdint.h>

#define FDI_ATE_USB_CS_EN 0
#define FDI_ATE_BS_EN 1

void fdi_gpio_initialize(void);

void fdi_gpio_on(uint32_t identifier);
void fdi_gpio_off(uint32_t identifier);

#endif
