#include "fd_gpio.h"

#include "apollo2.h"
#include <am_mcu_apollo.h>

void fd_gpio_configure_default(fd_gpio_t gpio) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_DISABLE);
}

void fd_gpio_configure_output(fd_gpio_t gpio) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_OUTPUT);
}

void fd_gpio_configure_output_open_drain(fd_gpio_t gpio) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_OUTPUT | AM_HAL_GPIO_OUT_OPENDRAIN);
}

void fd_gpio_configure_output_open_drain_pull_up(fd_gpio_t gpio) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_OUTPUT | AM_HAL_GPIO_OUT_OPENDRAIN | AM_HAL_GPIO_PULL12K);
}

void fd_gpio_configure_output_open_source_pull_down(fd_gpio_t gpio) {
    // !!! pull down not supported? -denis
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_INPUT);
}

void fd_gpio_configure_input(fd_gpio_t gpio) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_INPUT);
}

void fd_gpio_configure_input_pull_up(fd_gpio_t gpio) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_INPUT | AM_HAL_GPIO_PULL12K);
}

void fd_gpio_configure_input_pull_down(fd_gpio_t gpio) {
    // !!! pull down not supported? -denis
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_INPUT);
}

void fd_gpio_set(fd_gpio_t gpio, bool value) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
    if (value) {
        am_hal_gpio_out_bit_set(pin_number);
    } else {
        am_hal_gpio_out_bit_clear(pin_number);
    }
}

bool fd_gpio_get(fd_gpio_t gpio) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
    return (am_hal_gpio_input_read() & (1ULL << pin_number)) != 0;
}
