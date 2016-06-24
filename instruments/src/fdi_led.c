#include "fdi_led.h"

#include <stm32f4xx.h>

#define FDI_LED_PIN_B 13
#define FDI_LED_PIN_G 14
#define FDI_LED_PIN_R 15

void fdi_led_initialize(void) {
    RCC->AHB1ENR |= 1 << 2; // enable GPIOC clock
     // configure led pins as outputs (initialized high)
    GPIOC->BSRRL = 1 << FDI_LED_PIN_B;
    GPIOC->MODER |= 0b01 << (FDI_LED_PIN_B * 2);
    GPIOC->BSRRL = 1 << FDI_LED_PIN_G;
    GPIOC->MODER |= 0b01 << (FDI_LED_PIN_G * 2);
    GPIOC->BSRRL = 1 << FDI_LED_PIN_R;
    GPIOC->MODER |= 0b01 << (FDI_LED_PIN_R * 2);
}

void fdi_led_on(uint32_t pin) {
    GPIOC->BSRRH = 1 << pin;
}

void fdi_led_off(uint32_t pin) {
    GPIOC->BSRRL = 1 << pin;
}
