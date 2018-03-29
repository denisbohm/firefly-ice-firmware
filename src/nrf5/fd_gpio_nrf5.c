#include "fd_gpio.h"

#include "fd_nrf5.h"

static inline
NRF_GPIO_Type *fd_gpio_get_nrf_gpio(uint32_t port) {
    return (NRF_GPIO_Type *)(NRF_P0_BASE + port * 0x300UL);
}

static
void fd_gpio_configure(
    fd_gpio_t gpio,
    nrf_gpio_pin_dir_t dir,
    nrf_gpio_pin_input_t input,
    nrf_gpio_pin_pull_t pull,
    nrf_gpio_pin_drive_t drive,
    nrf_gpio_pin_sense_t sense
) {
    NRF_GPIO_Type *nrf_gpio = fd_gpio_get_nrf_gpio(gpio.port);
    nrf_gpio->PIN_CNF[gpio.pin] = ((uint32_t)dir << GPIO_PIN_CNF_DIR_Pos)
                                | ((uint32_t)input << GPIO_PIN_CNF_INPUT_Pos)
                                | ((uint32_t)pull << GPIO_PIN_CNF_PULL_Pos)
                                | ((uint32_t)drive << GPIO_PIN_CNF_DRIVE_Pos)
                                | ((uint32_t)sense << GPIO_PIN_CNF_SENSE_Pos);
}

void fd_gpio_configure_default(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        GPIO_PIN_CNF_DIR_Input,
        GPIO_PIN_CNF_INPUT_Disconnect,
        GPIO_PIN_CNF_PULL_Disabled,
        GPIO_PIN_CNF_DRIVE_S0S1,
        GPIO_PIN_CNF_SENSE_Disabled
    );
}

void fd_gpio_configure_output(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE
    );
}

void fd_gpio_configure_output_open_drain(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        GPIO_PIN_CNF_DIR_Output,
        GPIO_PIN_CNF_INPUT_Disconnect,
        GPIO_PIN_CNF_PULL_Disabled,
        GPIO_PIN_CNF_DRIVE_S0D1,
        GPIO_PIN_CNF_SENSE_Disabled
    );
}

void fd_gpio_configure_output_open_drain_pull_up(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        GPIO_PIN_CNF_DIR_Output,
        GPIO_PIN_CNF_INPUT_Disconnect,
        GPIO_PIN_CNF_PULL_Pullup,
        GPIO_PIN_CNF_DRIVE_S0D1,
        GPIO_PIN_CNF_SENSE_Disabled
    );
}

void fd_gpio_configure_output_open_source_pull_down(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        GPIO_PIN_CNF_DIR_Output,
        GPIO_PIN_CNF_INPUT_Disconnect,
        GPIO_PIN_CNF_PULL_Pulldown,
        GPIO_PIN_CNF_DRIVE_D0S1,
        GPIO_PIN_CNF_SENSE_Disabled
    );
}

void fd_gpio_configure_input(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        NRF_GPIO_PIN_DIR_INPUT,
        NRF_GPIO_PIN_INPUT_CONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE
    );
}

void fd_gpio_configure_input_pull_up(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        NRF_GPIO_PIN_DIR_INPUT,
        NRF_GPIO_PIN_INPUT_CONNECT,
        NRF_GPIO_PIN_PULLUP,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE
    );
}

void fd_gpio_configure_input_pull_down(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        NRF_GPIO_PIN_DIR_INPUT,
        NRF_GPIO_PIN_INPUT_CONNECT,
        NRF_GPIO_PIN_PULLDOWN,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE
    );
}

void fd_gpio_set(fd_gpio_t gpio, bool value) {
    NRF_GPIO_Type *nrf_gpio = fd_gpio_get_nrf_gpio(gpio.port);
    if (value) {
        nrf_gpio->OUTSET = 1UL << gpio.pin;
    } else {
        nrf_gpio->OUTCLR = 1UL << gpio.pin;
    }
}

bool fd_gpio_get(fd_gpio_t gpio) {
    NRF_GPIO_Type *nrf_gpio = fd_gpio_get_nrf_gpio(gpio.port);
    return ((nrf_gpio->IN >> gpio.pin) & 1UL) != 0;
}