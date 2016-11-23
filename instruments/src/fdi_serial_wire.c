#include "fdi_serial_wire.h"

#include "fdi_delay.h"
#include "fdi_gpio.h"

#include "fd_log.h"

fdi_serial_wire_t fdi_serial_wires[fdi_serial_wire_count];

fdi_serial_wire_t *fdi_serial_wire_get(uint32_t index) {
    return &fdi_serial_wires[index];
}

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

bool fdi_serial_wire_get_reset(fdi_serial_wire_t *serial_wire) {
    return fdi_gpio_get(serial_wire->gpio_sense_reset);
}

// SWDCLK has a pull down resistor
// SWDIO has a pull up resistor
// SWD Master writes and reads data on falling clock edge

#define fdi_serial_wire_half_bit_delay() // fdi_delay_ns(50)

void fdi_serial_wire_shift_out(fdi_serial_wire_t *serial_wire, uint8_t byte, int bit_count) {
    while (bit_count-- > 0) {
        fdi_serial_wire_half_bit_delay();
        fdi_gpio_on(serial_wire->gpio_clock);
        fdi_serial_wire_half_bit_delay();

        if (byte & 1) {
            fdi_gpio_on(serial_wire->gpio_data);
        } else {
            fdi_gpio_off(serial_wire->gpio_data);
        }
        byte >>= 1;

        fdi_gpio_off(serial_wire->gpio_clock);
    }
}

void fdi_serial_wire_shift_out_bytes(fdi_serial_wire_t *serial_wire, uint8_t *data, uint32_t length) {
    fdi_gpio_serial_wire_debug_out(serial_wire->gpio_clock, serial_wire->gpio_data, data, length);
}

uint8_t fdi_serial_wire_shift_in(fdi_serial_wire_t *serial_wire, int bit_count) {
    uint8_t byte = 0;
    while (bit_count-- > 0) {
        fdi_serial_wire_half_bit_delay();
        fdi_gpio_on(serial_wire->gpio_clock);
        fdi_serial_wire_half_bit_delay();

        byte >>= 1;
        if (fdi_gpio_get(serial_wire->gpio_data)) {
            byte |= 0b10000000;
        }

        fdi_gpio_off(serial_wire->gpio_clock);
    }
    return byte;
}

void fdi_serial_wire_shift_in_bytes(fdi_serial_wire_t *serial_wire, uint8_t *data, uint32_t length) {
    fdi_gpio_serial_wire_debug_in(serial_wire->gpio_clock, serial_wire->gpio_data, data, length);
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
        serial_wire->gpio_sense_reset = FDI_GPIO_ATE_SWD1_SENSE_NRESET;
        serial_wire->overrun_detection_enabled = false;
        serial_wire->ack_wait_retry_count = 3;
        serial_wire->register_retry_count = 3;
    }
    {
        fdi_serial_wire_t *serial_wire = &fdi_serial_wires[1];
        serial_wire->gpio_power = FDI_GPIO_ATE_SWD2_PWR;
        serial_wire->gpio_reset = FDI_GPIO_ATE_SWD2_NRESET;
        serial_wire->gpio_direction = FDI_GPIO_ATE_SWD2_DIR_TO_DUT;
        serial_wire->gpio_clock = FDI_GPIO_ATE_SWD2_SWDCLK;
        serial_wire->gpio_data = FDI_GPIO_ATE_SWD2_SWDIO;
        serial_wire->gpio_sense_reset = FDI_GPIO_ATE_SWD2_SENSE_NRESET;
        serial_wire->overrun_detection_enabled = false;
        serial_wire->ack_wait_retry_count = 3;
        serial_wire->register_retry_count = 3;
    }
}