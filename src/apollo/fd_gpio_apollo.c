#include "fd_gpio.h"

#include "fd_apollo.h"

void fd_gpio_configure_default(fd_gpio_t gpio) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
#ifdef AM_PART_APOLLO2
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_DISABLE);
#else
    am_hal_gpio_pinconfig(pin_number, g_AM_HAL_GPIO_DISABLE);
#endif
}

void fd_gpio_configure_output(fd_gpio_t gpio) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
#ifdef AM_PART_APOLLO2
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_OUTPUT);
#else
    am_hal_gpio_pinconfig(pin_number, g_AM_HAL_GPIO_OUTPUT);
#endif
}

void fd_gpio_configure_output_open_drain(fd_gpio_t gpio) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
#ifdef AM_PART_APOLLO2
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_OUTPUT | AM_HAL_GPIO_OUT_OPENDRAIN);
#else
    am_hal_gpio_pincfg_t pincfg = {
        .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
        .eGPOutcfg      = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
    };
    am_hal_gpio_pinconfig(pin_number, pincfg);
#endif
}

void fd_gpio_configure_output_open_drain_pull_up(fd_gpio_t gpio) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
#ifdef AM_PART_APOLLO2
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_OUTPUT | AM_HAL_GPIO_OUT_OPENDRAIN | AM_HAL_GPIO_PULL12K);
#else
    am_hal_gpio_pincfg_t pincfg = {
        .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
        .ePullup        = AM_HAL_GPIO_PIN_PULLUP_12K,
        .eGPOutcfg      = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
    };
    am_hal_gpio_pinconfig(pin_number, pincfg);
#endif
}

void fd_gpio_configure_output_open_source_pull_down(fd_gpio_t gpio) {
    // !!! pull down not supported? -denis
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
#ifdef AM_PART_APOLLO2
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_INPUT);
#else
    am_hal_gpio_pinconfig(pin_number, g_AM_HAL_GPIO_INPUT);
#endif
}

void fd_gpio_configure_input(fd_gpio_t gpio) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
#ifdef AM_PART_APOLLO2
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_INPUT);
#else
    am_hal_gpio_pinconfig(pin_number, g_AM_HAL_GPIO_INPUT);
#endif
}

void fd_gpio_configure_input_pull_up(fd_gpio_t gpio) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
#ifdef AM_PART_APOLLO2
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_INPUT | AM_HAL_GPIO_PULL12K);
#else
    am_hal_gpio_pinconfig(pin_number, g_AM_HAL_GPIO_INPUT_PULLUP_12);
#endif
}

void fd_gpio_configure_input_pull_down(fd_gpio_t gpio) {
    // !!! pull down not supported? -denis
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
#ifdef AM_PART_APOLLO2
    am_hal_gpio_pin_config(pin_number, AM_HAL_GPIO_INPUT);
#else
    am_hal_gpio_pinconfig(pin_number, g_AM_HAL_GPIO_INPUT);
#endif
}

void fd_gpio_set(fd_gpio_t gpio, bool value) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
#ifdef AM_PART_APOLLO2
    if (value) {
        am_hal_gpio_out_bit_set(pin_number);
    } else {
        am_hal_gpio_out_bit_clear(pin_number);
    }
#else
    if (value) {
        am_hal_gpio_output_set(pin_number);
    } else {
        am_hal_gpio_output_clear(pin_number);
    }
#endif
}

bool fd_gpio_get(fd_gpio_t gpio) {
    uint32_t pin_number = gpio.port * 32 + gpio.pin;
#ifdef AM_PART_APOLLO2
    return (am_hal_gpio_input_read() & (1ULL << pin_number)) != 0;
#else
    return am_hal_gpio_input_read(pin_number) != 0;
#endif
}
