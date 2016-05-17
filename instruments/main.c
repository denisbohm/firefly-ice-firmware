#include "fdi_usb.h"

#include <stm32f4xx.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define FDI_LED_PIN_B 13
#define FDI_LED_PIN_G 14
#define FDI_LED_PIN_R 15

static void fdi_led_initialize(void) {
    RCC->AHB1ENR |= 1 << 2; // enable GPIOC clock
     // configure led pins as outputs (initialized high)
    GPIOC->BSRRL = 1 << FDI_LED_PIN_B;
    GPIOC->MODER |= 0b01 << (FDI_LED_PIN_B * 2);
    GPIOC->BSRRL = 1 << FDI_LED_PIN_G;
    GPIOC->MODER |= 0b01 << (FDI_LED_PIN_G * 2);
    GPIOC->BSRRL = 1 << FDI_LED_PIN_R;
    GPIOC->MODER |= 0b01 << (FDI_LED_PIN_R * 2);
}

static void fdi_led_on(uint32_t pin) {
    GPIOC->BSRRH = 1 << pin;
}

static void fdi_led_off(uint32_t pin) {
    GPIOC->BSRRL = 1 << pin;
}

#define FDI_ATE_MCU_VCC_SENSE_PORT D
#define FDI_ATE_MCU_VCC_SENSE_PIN 2

static void fdi_relay_initialize(void) {
    RCC->AHB1ENR |= 1 << 3; // enable GPIOD clock
     // configure relay pins as outputs (initialized high)
    GPIOD->BSRRH = 1 << FDI_ATE_MCU_VCC_SENSE_PIN;
    GPIOD->MODER |= 0b01 << (FDI_ATE_MCU_VCC_SENSE_PIN * 2);
}

static void fdi_relay_on(uint32_t pin) {
    GPIOD->BSRRL = 1 << pin;
}

static void fdi_relay_off(uint32_t pin) {
    GPIOD->BSRRH = 1 << pin;
}

static void fdi_start_high_speed_clock(void) {
    RCC_HSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET);
    RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);

    FLASH_SetLatency(FLASH_Latency_2);

    RCC_PLLCmd(DISABLE);
    RCC_PLLConfig(RCC_PLLSource_HSI, 16, 336, 4, 7); // 84MHz & 48MHz
    RCC_PLLCmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
}

static bool rx = false;

static void rx_callback(uint8_t *buffer, size_t length) {
    rx = true;
}

void main(void) {
    fdi_led_initialize();
    fdi_relay_initialize();

    fdi_start_high_speed_clock();

    fdi_usb_initialize();
    fdi_usb_set_data_callback(rx_callback);
    fdi_usb_power_up();

    uint8_t data[64] = {0x01, 0x02, };
    while (true) {
        if (rx) {
            fdi_usb_send(data, sizeof(data));
            rx = false;
        }
    }

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
