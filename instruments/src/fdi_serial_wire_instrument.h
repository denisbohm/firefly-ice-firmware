#ifndef FDI_SERIAL_WIRE_INSTRUMENT_H
#define FDI_SERIAL_WIRE_INSTRUMENT_H

#include "fdi_instrument.h"
#include "fdi_serial_wire.h"

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

#endif