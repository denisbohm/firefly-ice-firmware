#ifndef FDI_RELAY_H
#define FDI_RELAY_H

#include <stdint.h>

#define FDI_ATE_USB_5V_EN     0
#define FDI_ATE_USB_D_EN      1
#define FDI_ATE_BATTERY_SENSE 2
#define FDI_ATE_BUTTON_EN     3
#define FDI_ATE_MCU_VCC_SENSE 4

void fdi_relay_initialize(void);

void fdi_relay_on(uint32_t identifier);
void fdi_relay_off(uint32_t identifier);

#endif
