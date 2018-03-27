#include "fd_gpio.h"

#include "fd_nrf5.h"

void fd_gpio_configure_default(fd_gpio_t gpio) {
    uint32_t pin_number = NRF_GPIO_PIN_MAP(gpio.port, gpio.pin);
    NRF_GPIO->PIN_CNF[pin_number] =
        (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos) |
        (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos) |
        (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos) |
        (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
        (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos);
}

void fd_gpio_configure_output(fd_gpio_t gpio) {
    uint32_t pin_number = NRF_GPIO_PIN_MAP(gpio.port, gpio.pin);
    nrf_gpio_cfg_output(pin_number);
}

void fd_gpio_configure_output_open_drain(fd_gpio_t gpio) {
    uint32_t pin_number = NRF_GPIO_PIN_MAP(gpio.port, gpio.pin);
    NRF_GPIO->PIN_CNF[pin_number] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
                                            | (GPIO_PIN_CNF_DRIVE_S0D1 << GPIO_PIN_CNF_DRIVE_Pos)
                                            | (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
                                            | (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos)
                                            | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
}

void fd_gpio_configure_output_open_drain_pull_up(fd_gpio_t gpio) {
    uint32_t pin_number = NRF_GPIO_PIN_MAP(gpio.port, gpio.pin);
    NRF_GPIO->PIN_CNF[pin_number] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
                                            | (GPIO_PIN_CNF_DRIVE_S0D1 << GPIO_PIN_CNF_DRIVE_Pos)
                                            | (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos)
                                            | (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos)
                                            | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
}

void fd_gpio_configure_output_open_source_pull_down(fd_gpio_t gpio) {
    uint32_t pin_number = NRF_GPIO_PIN_MAP(gpio.port, gpio.pin);
    NRF_GPIO->PIN_CNF[pin_number] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
                                            | (GPIO_PIN_CNF_DRIVE_D0S1 << GPIO_PIN_CNF_DRIVE_Pos)
                                            | (GPIO_PIN_CNF_PULL_Pulldown << GPIO_PIN_CNF_PULL_Pos)
                                            | (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos)
                                            | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
}

void fd_gpio_configure_input(fd_gpio_t gpio) {
    uint32_t pin_number = NRF_GPIO_PIN_MAP(gpio.port, gpio.pin);
    nrf_gpio_cfg_input(pin_number, NRF_GPIO_PIN_NOPULL);
}

void fd_gpio_configure_input_pull_up(fd_gpio_t gpio) {
    uint32_t pin_number = NRF_GPIO_PIN_MAP(gpio.port, gpio.pin);
    nrf_gpio_cfg_input(pin_number, NRF_GPIO_PIN_PULLUP);
}

void fd_gpio_configure_input_pull_down(fd_gpio_t gpio) {
    uint32_t pin_number = NRF_GPIO_PIN_MAP(gpio.port, gpio.pin);
    nrf_gpio_cfg_input(pin_number, NRF_GPIO_PIN_PULLDOWN);
}

void fd_gpio_set(fd_gpio_t gpio, bool value) {
    uint32_t pin_number = NRF_GPIO_PIN_MAP(gpio.port, gpio.pin);
    nrf_gpio_pin_write(pin_number, value ? 1 : 0);
}

bool fd_gpio_get(fd_gpio_t gpio) {
    uint32_t pin_number = NRF_GPIO_PIN_MAP(gpio.port, gpio.pin);
    return nrf_gpio_pin_read(pin_number) != 0;
}