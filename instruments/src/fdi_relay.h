#ifndef FDI_RELAY_H
#define FDI_RELAY_H

#include "fdi_gpio.h"

#define FDI_RELAY_ATE_USB_5V_EN     FDI_GPIO_ATE_USB_5V_EN
#define FDI_RELAY_ATE_USB_D_EN      FDI_GPIO_ATE_USB_D_EN
#define FDI_RELAY_ATE_BATTERY_SENSE FDI_GPIO_ATE_BATTERY_SENSE
#define FDI_RELAY_ATE_BUTTON_EN     FDI_GPIO_ATE_BUTTON_EN
#define FDI_RELAY_ATE_MCU_VCC_SENSE FDI_GPIO_ATE_MCU_VCC_SENSE

void fdi_relay_initialize(void);

void fdi_relay_set(uint32_t identifier, bool value);
void fdi_relay_on(uint32_t identifier);
void fdi_relay_off(uint32_t identifier);

#endif
