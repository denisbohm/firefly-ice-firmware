#ifndef FDI_LED_H
#define FDI_LED_H

#include <stdint.h>

#define FDI_LED_PIN_B 13
#define FDI_LED_PIN_G 14
#define FDI_LED_PIN_R 15

void fdi_led_initialize(void);

void fdi_led_on(uint32_t pin);
void fdi_led_off(uint32_t pin);

#endif
