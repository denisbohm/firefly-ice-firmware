#ifndef FDI_SERIAL_WIRE_INSTRUMENT_H
#define FDI_SERIAL_WIRE_INSTRUMENT_H

#include "fdi_instrument.h"
#include "fdi_serial_wire_debug.h"

#define fdi_serial_wire_tx_data_size 256

typedef struct {
    fdi_instrument_t super;
    fdi_serial_wire_t *serial_wire;
    uint8_t tx_data[fdi_serial_wire_tx_data_size];
    uint32_t tx_data_index;

} fdi_serial_wire_instrument_t;

void fdi_serial_wire_instrument_initialize(void);

uint32_t fdi_serial_wire_instrument_get_count(void);
fdi_serial_wire_instrument_t *fdi_serial_wire_instrument_get_at(uint32_t index);

bool fdi_serial_wire_instrument_write_memory(
    fdi_serial_wire_instrument_t *instrument,
    uint32_t address,
    uint8_t *data,
    uint32_t length,
    fdi_serial_wire_debug_error_t *error
);

bool fdi_serial_wire_instrument_read_memory(
    fdi_serial_wire_instrument_t *instrument,
    uint32_t address,
    uint8_t *data,
    uint32_t length,
    fdi_serial_wire_debug_error_t *error
);

void fdi_serial_wire_instrument_set_half_bit_delay(
    fdi_serial_wire_instrument_t *instrument,
    uint32_t half_bit_delay_ns
);

void fdi_serial_wire_instrument_set_target_id(
    fdi_serial_wire_instrument_t *instrument,
    uint32_t target_id
);

void fdi_serial_wire_instrument_set_access_port_id(
    fdi_serial_wire_instrument_t *instrument,
    uint8_t access_port_id
);

bool fdi_serial_wire_instrument_connect(
    fdi_serial_wire_instrument_t *instrument,
    uint32_t *dpid,
    fdi_serial_wire_debug_error_t *error
);

#endif