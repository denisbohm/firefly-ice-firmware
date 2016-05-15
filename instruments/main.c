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

static void fdi_led_on(uint32_t pin) {
    GPIOC->BSRR = (1 << 16) << pin;
}

static void fdi_led_off(uint32_t pin) {
    GPIOC->BSRR = 1 << pin;
}

#define FDI_ATE_MCU_VCC_SENSE_PORT D
#define FDI_ATE_MCU_VCC_SENSE_PIN 2

static void fdi_relay_initialize(void) {
    RCC->AHB1ENR |= 1 << 3; // enable GPIOD clock
     // configure relay pins as outputs (initialized high)
    GPIOD->BSRR = (1 << 16) << FDI_ATE_MCU_VCC_SENSE_PIN;
    GPIOD->MODER |= 0b01 << (FDI_ATE_MCU_VCC_SENSE_PIN * 2);
}

static void fdi_relay_on(uint32_t pin) {
    GPIOD->BSRR = 1 << pin;
}

static void fdi_relay_off(uint32_t pin) {
    GPIOD->BSRR = (1 << 16) << pin;
}

void main(void) {
    fdi_led_initialize();
    fdi_relay_initialize();
    fdi_led_on(FDI_LED_PIN_R);
    fdi_led_off(FDI_LED_PIN_R);
    fdi_relay_on(FDI_ATE_MCU_VCC_SENSE_PIN);
    fdi_relay_off(FDI_ATE_MCU_VCC_SENSE_PIN);
    fdi_led_on(FDI_LED_PIN_G);
    fdi_led_off(FDI_LED_PIN_G);
    fdi_led_on(FDI_LED_PIN_B);
    fdi_led_off(FDI_LED_PIN_B);

    exit(0);
}
