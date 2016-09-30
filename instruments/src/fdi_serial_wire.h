#ifndef FDI_SERIAL_WIRE_H
#define FDI_SERIAL_WIRE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t gpio_power;
    uint32_t gpio_reset;
    uint32_t gpio_direction;
    uint32_t gpio_clock;
    uint32_t gpio_data;
    uint32_t gpio_sense_reset;

    bool overrun_detection_enabled;
    uint32_t ack_wait_retry_count;
    uint32_t register_retry_count;
} fdi_serial_wire_t;

#define fdi_serial_wire_count 2

extern fdi_serial_wire_t fdi_serial_wires[];

void fdi_serial_wire_initialize(void);

fdi_serial_wire_t *fdi_serial_wire_get(uint32_t index);

void fdi_serial_wire_reset(fdi_serial_wire_t *serial_wire);

void fdi_serial_wire_set_power(fdi_serial_wire_t *serial_wire, bool power);

void fdi_serial_wire_set_reset(fdi_serial_wire_t *serial_wire, bool nreset);

bool fdi_serial_wire_get_reset(fdi_serial_wire_t *serial_wire);

void fdi_serial_wire_set_direction_to_read(fdi_serial_wire_t *serial_wire);

void fdi_serial_wire_set_direction_to_write(fdi_serial_wire_t *serial_wire);

void fdi_serial_wire_shift_out(fdi_serial_wire_t *serial_wire, uint8_t byte, int bit_count);

uint8_t fdi_serial_wire_shift_in(fdi_serial_wire_t *serial_wire, int bit_count);

#endif
