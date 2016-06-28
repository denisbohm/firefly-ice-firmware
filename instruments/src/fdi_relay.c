#include "fdi_relay.h"

#include "fd_log.h"

#include <stm32f4xx.h>

typedef struct {
    uint32_t enable;
    GPIO_TypeDef *port;
    uint8_t pin;
} fdi_relay_t;

fdi_relay_t fdi_relays[] = {
    { // ATE_USB_5V_EN
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 13
    },
    { // ATE_USB_D_EN
        .enable = RCC_AHB1Periph_GPIOB,
        .port = GPIOB,
        .pin  = 15
    },
    { // ATE_BATTERY_SENSE
        .enable = RCC_AHB1Periph_GPIOC,
        .port = GPIOC,
        .pin  = 10
    },
    { // ATE_BUTTON_EN
        .enable = RCC_AHB1Periph_GPIOC,
        .port = GPIOC,
        .pin  = 12
    },
    { // ATE_MCU_VCC_SENSE
        .enable = RCC_AHB1Periph_GPIOD,
        .port = GPIOD,
        .pin  = 2
    },
};

const int fdi_relay_count = (sizeof(fdi_relays) / sizeof(fdi_relay_t));

void fdi_relay_initialize(void) {
    for (int i = 0; i < fdi_relay_count; ++i) {
        fdi_relay_t *relay = &fdi_relays[i];

        // enable gpio port clock
        RCC->AHB1ENR |= relay->enable;

        // configure relay gpio control pins as outputs (initialized high)
        relay->port->BSRRH = 1 << relay->pin;
        relay->port->MODER |= 0b01 << (relay->pin * 2);
    }
}

void fdi_relay_on(uint32_t identifier) {
    fd_log_assert(identifier < fdi_relay_count);

    fdi_relay_t *relay = &fdi_relays[identifier];
    relay->port->BSRRL = 1 << relay->pin;
}

void fdi_relay_off(uint32_t identifier) {
    fd_log_assert(identifier < fdi_relay_count);

    fdi_relay_t *relay = &fdi_relays[identifier];
    relay->port->BSRRH = 1 << relay->pin;
}
