#include "fdi_gpio.h"

#include "fd_log.h"

#include <stm32f4xx.h>

typedef struct {
    uint32_t enable;
    GPIO_TypeDef *port;
    uint8_t pin;
} fdi_gpio_t;

fdi_gpio_t fdi_gpios[] = {
    { // ATE_USB_CS_EN
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 14
    },
    { // FDI_ATE_BS_EN
        .enable = RCC_AHB1Periph_GPIOC,
        .port = GPIOC,
        .pin  = 11
    },
};

const int fdi_gpio_count = (sizeof(fdi_gpios) / sizeof(fdi_gpio_t));

void fdi_gpio_initialize(void) {
    for (int i = 0; i < fdi_gpio_count; ++i) {
        fdi_gpio_t *gpio = &fdi_gpios[i];

        // enable gpio port clock
        RCC->AHB1ENR |= gpio->enable;

        // configure gpio control pins as outputs (initialized low)
        gpio->port->BSRRH = 1 << gpio->pin;
        gpio->port->MODER |= 0b01 << (gpio->pin * 2);
    }
}

void fdi_gpio_on(uint32_t identifier) {
    fd_log_assert(identifier < fdi_gpio_count);

    fdi_gpio_t *gpio = &fdi_gpios[identifier];
    gpio->port->BSRRL = 1 << gpio->pin;
}

void fdi_gpio_off(uint32_t identifier) {
    fd_log_assert(identifier < fdi_gpio_count);

    fdi_gpio_t *gpio = &fdi_gpios[identifier];
    gpio->port->BSRRH = 1 << gpio->pin;
}
