#include "fdi_serial_wire.h"

#include "fdi_delay.h"
#include "fdi_gpio.h"

fdi_serial_wire_t fdi_serial_wires[fdi_serial_wire_count];

void fdi_serial_wire_set_power(fdi_serial_wire_t *serial_wire, bool power) {
    if (power) {
        fdi_gpio_on(serial_wire->gpio_power);
    } else {
        fdi_gpio_off(serial_wire->gpio_power);
    }
}

void fdi_serial_wire_set_reset(fdi_serial_wire_t *serial_wire, bool reset) {
    if (reset) {
        fdi_gpio_on(serial_wire->gpio_reset);
    } else {
        fdi_gpio_off(serial_wire->gpio_reset);
    }
}

static
void fdi_serial_wire_half_bit_delay(void) {
    fdi_delay_us(5);
}

void fdi_serial_wire_shift_out(fdi_serial_wire_t *serial_wire, int byte, int bit_count) {
    while (bit_count-- > 0) {
        fdi_serial_wire_half_bit_delay();
        fdi_gpio_on(serial_wire->gpio_clock);
        if (byte & 1) {
            fdi_gpio_on(serial_wire->gpio_data);
        } else {
            fdi_gpio_off(serial_wire->gpio_data);
        }
        fdi_serial_wire_half_bit_delay();
        fdi_gpio_off(serial_wire->gpio_clock);
        byte >>= 1;
    }
}

uint8_t fdi_serial_wire_shift_in(fdi_serial_wire_t *serial_wire, int bit_count) {
    uint8_t byte = 0;
    while (bit_count-- > 0) {
        byte >>= 1;
        fdi_serial_wire_half_bit_delay();
        fdi_gpio_on(serial_wire->gpio_clock);
        if (fdi_gpio_get(serial_wire->gpio_data)) {
            byte |= 0b10000000;
        }
        fdi_serial_wire_half_bit_delay();
        fdi_gpio_off(serial_wire->gpio_clock);
    }
    return byte;
}

void fdi_serial_wire_set_direction_to_read(fdi_serial_wire_t *serial_wire) {
    fdi_gpio_set_mode_in(serial_wire->gpio_data);
    fdi_gpio_off(serial_wire->gpio_direction);
}

void fdi_serial_wire_set_direction_to_write(fdi_serial_wire_t *serial_wire) {
    fdi_gpio_on(serial_wire->gpio_data);
    fdi_gpio_on(serial_wire->gpio_direction);
    fdi_gpio_set_mode_out(serial_wire->gpio_data);
}

void fdi_serial_wire_reset(fdi_serial_wire_t *serial_wire) {
    fdi_gpio_off(serial_wire->gpio_power);
    fdi_gpio_set_mode_in(serial_wire->gpio_data);
    fdi_gpio_off(serial_wire->gpio_direction);
    fdi_gpio_on(serial_wire->gpio_clock);
    fdi_gpio_off(serial_wire->gpio_reset);
}

void fdi_serial_wire_initialize(void) {
    {
        fdi_serial_wire_t *serial_wire = &fdi_serial_wires[0];
        serial_wire->gpio_power = FDI_GPIO_ATE_SWD1_PWR;
        serial_wire->gpio_reset = FDI_GPIO_ATE_SWD1_NRESET;
        serial_wire->gpio_direction = FDI_GPIO_ATE_SWD1_DIR_TO_DUT;
        serial_wire->gpio_clock = FDI_GPIO_ATE_SWD1_SWDCLK;
        serial_wire->gpio_data = FDI_GPIO_ATE_SWD1_SWDIO;
    }
    {
        fdi_serial_wire_t *serial_wire = &fdi_serial_wires[1];
        serial_wire->gpio_power = FDI_GPIO_ATE_SWD2_PWR;
        serial_wire->gpio_reset = FDI_GPIO_ATE_SWD2_NRESET;
        serial_wire->gpio_direction = FDI_GPIO_ATE_SWD2_DIR_TO_DUT;
        serial_wire->gpio_clock = FDI_GPIO_ATE_SWD2_SWDCLK;
        serial_wire->gpio_data = FDI_GPIO_ATE_SWD2_SWDIO;
    }
}