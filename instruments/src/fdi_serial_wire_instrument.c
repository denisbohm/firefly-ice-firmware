#include "fdi_serial_wire_instrument.h"

#include "fdi_api.h"
#include "fdi_gpio.h"
#include "fdi_instruments.h"
#include "fdi_serial_wire.h"

#include "fd_log.h"

#define fdi_serial_wire_tx_data_size 256

typedef struct {
    uint64_t identifier;
    fdi_serial_wire_t *serial_wire;
    uint8_t tx_data[fdi_serial_wire_tx_data_size];
    uint32_t tx_data_index;

} fdi_serial_wire_instrument_t;

#define fdi_serial_wire_instrument_count 1

static const uint64_t apiTypeSetOutputs = 1;
static const uint64_t apiTypeGetInputs = 2;
static const uint64_t apiTypeShiftOutBits = 3;
static const uint64_t apiTypeShiftOutData = 4;
static const uint64_t apiTypeShiftInBits = 5;
static const uint64_t apiTypeShiftInData = 6;
static const uint64_t apiTypeFlush = 7;
static const uint64_t apiTypeData = 8;

static const uint8_t outputReset = 0;
static const uint8_t outputDirection = 1;

fdi_serial_wire_instrument_t fdi_serial_wire_instruments[fdi_serial_wire_instrument_count];

fdi_serial_wire_instrument_t *fdi_serial_wire_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_serial_wire_instrument_count; ++i) {
        fdi_serial_wire_instrument_t *instrument = &fdi_serial_wire_instruments[i];
        if (instrument->identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}

void fdi_serial_wire_instrument_queue_byte(fdi_serial_wire_instrument_t *instrument, uint8_t byte) {
    if (instrument->tx_data_index < fdi_serial_wire_tx_data_size) {
        instrument->tx_data[instrument->tx_data_index++] = byte;
    } else {
        fd_log_assert_fail("overflow");
    }
}

void fdi_serial_wire_instrument_flush(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    if (instrument->tx_data_index > 0) {
        if (fdi_api_send(instrument->identifier, apiTypeData, instrument->tx_data, instrument->tx_data_index)) {
            instrument->tx_data_index = 0;
        } else {
            fd_log_assert_fail("can't send");
        }
    }
}

void fdi_serial_wire_instrument_set_outputs(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    uint8_t outputs = fd_binary_get_uint8(binary);
    uint8_t values = fd_binary_get_uint8(binary);

    if (outputs & outputReset) {
        fdi_serial_wire_set_nreset(instrument->serial_wire, (values & outputReset) != 0);
    }
    if (outputs & outputDirection) {
        if ((values & outputDirection) != 0) {
            fdi_serial_wire_set_direction_to_write(instrument->serial_wire);
        } else {
            fdi_serial_wire_set_direction_to_read(instrument->serial_wire);
        }
    }
}

void fdi_serial_wire_instrument_get_inputs(uint64_t identifier __attribute__((unused)), uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_serial_wire_instrument_queue_byte(instrument, 0x00); // not supported currently -denis
}

void fdi_serial_wire_instrument_shift_out_bits(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    // bit_count is number of bits - 1
    uint8_t bit_count = fd_binary_get_uint8(binary);
    uint8_t byte = fd_binary_get_uint8(binary);
    fdi_serial_wire_shift_out(instrument->serial_wire, byte, bit_count + 1);
}

void fdi_serial_wire_instrument_shift_out_data(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    // byte_count is number of bytes - 1
    uint32_t byte_count = (uint32_t)fd_binary_get_varuint(binary);
    do {
        uint8_t byte = fd_binary_get_uint8(binary);
        fdi_serial_wire_shift_out(instrument->serial_wire, byte, 8);
    } while (byte_count-- > 0);
}

void fdi_serial_wire_instrument_shift_in_bits(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    // bit_count is number of bits - 1
    uint8_t bit_count = fd_binary_get_uint8(binary);
    uint8_t byte = fdi_serial_wire_shift_in(instrument->serial_wire, bit_count + 1);
    fdi_serial_wire_instrument_queue_byte(instrument, byte);
}

void fdi_serial_wire_instrument_shift_in_data(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    // byte_count is number of bytes - 1
    uint32_t byte_count = (uint32_t)fd_binary_get_varuint(binary);
    do {
        uint8_t byte = fdi_serial_wire_shift_in(instrument->serial_wire, 8);
        fdi_serial_wire_instrument_queue_byte(instrument, byte);
    } while (byte_count-- > 0);
}

void fdi_serial_wire_instrument_initialize(void) {
    for (int i = 0; i < fdi_serial_wire_count; ++i) {
        fdi_serial_wire_instrument_t *instrument = &fdi_serial_wire_instruments[i];
        uint64_t identifier = fdi_instruments_register();
        instrument->identifier = identifier;
        instrument->serial_wire = &fdi_serial_wires[i];
        instrument->tx_data_index = 0;
        fdi_api_register(identifier, apiTypeSetOutputs, fdi_serial_wire_instrument_set_outputs);
        fdi_api_register(identifier, apiTypeGetInputs, fdi_serial_wire_instrument_get_inputs);
        fdi_api_register(identifier, apiTypeShiftOutBits, fdi_serial_wire_instrument_shift_out_bits);
        fdi_api_register(identifier, apiTypeShiftOutData, fdi_serial_wire_instrument_shift_out_data);
        fdi_api_register(identifier, apiTypeShiftInBits, fdi_serial_wire_instrument_shift_in_bits);
        fdi_api_register(identifier, apiTypeShiftInData, fdi_serial_wire_instrument_shift_in_data);
        fdi_api_register(identifier, apiTypeFlush, fdi_serial_wire_instrument_flush);
    }
}