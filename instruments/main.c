#include <stm32f4xx.h>

#include <stdlib.h>

#define FDI_LED_PIN_B 13
#define FDI_LED_PIN_G 14
#define FDI_LED_PIN_R 15

static void fdi_led_initialize(void) {
    RCC->AHB1ENR |= 1 << 2; // enable GPIOC clock
     // configure led pins as outputs (initialized high)
    GPIOC->BSRR = 1 << FDI_LED_PIN_B;
    GPIOC->MODER |= 0b01 << (FDI_LED_PIN_B * 2);
    GPIOC->BSRR = 1 << FDI_LED_PIN_G;
    GPIOC->MODER |= 0b01 << (FDI_LED_PIN_G * 2);
    GPIOC->BSRR = 1 << FDI_LED_PIN_R;
    GPIOC->MODER |= 0b01 << (FDI_LED_PIN_R * 2);
}

static void fdi_led_on(uint32_t led) {
    GPIOC->BSRR = (1 << 16) << led;
}

static void fdi_led_off(uint32_t led) {
    GPIOC->BSRR = 1 << led;
}

void main(void) {
    fdi_led_initialize();
    fdi_led_on(FDI_LED_PIN_R);
    fdi_led_off(FDI_LED_PIN_R);
    fdi_led_on(FDI_LED_PIN_G);
    fdi_led_off(FDI_LED_PIN_G);
    fdi_led_on(FDI_LED_PIN_B);
    fdi_led_off(FDI_LED_PIN_B);

    exit(0);
}
