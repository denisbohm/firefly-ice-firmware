#include "fdi_serial_wire_instrument.h"

#include "fdi_api.h"
#include "fdi_gpio.h"
#include "fdi_instrument.h"
#include "fdi_serial_wire.h"
#include "fdi_serial_wire_debug.h"
#include "fdi_s25fl116k.h"

#include "fd_log.h"

#include <string.h>

static const uint64_t apiTypeReset = 0;
static const uint64_t apiTypeSetOutputs = 1;
static const uint64_t apiTypeGetInputs = 2;
static const uint64_t apiTypeShiftOutBits = 3;
static const uint64_t apiTypeShiftOutData = 4;
static const uint64_t apiTypeShiftInBits = 5;
static const uint64_t apiTypeShiftInData = 6;
static const uint64_t apiTypeFlush = 7;
static const uint64_t apiTypeData = 8;
static const uint64_t apiTypeSetEnabled = 9;
static const uint64_t apiTypeWriteMemory = 10;
static const uint64_t apiTypeReadMemory = 11;
static const uint64_t apiTypeWriteFromStorage = 12;
static const uint64_t apiTypeCompareToStorage = 13;
static const uint64_t apiTypeTransfer = 14;

static const uint8_t outputIndicator = 0;
static const uint8_t outputReset = 1;
static const uint8_t outputDirection = 2;

static const uint8_t inputReset = 0;

#define fdi_serial_wire_instrument_count 2
fdi_serial_wire_instrument_t fdi_serial_wire_instruments[fdi_serial_wire_instrument_count];

uint32_t fdi_serial_wire_instrument_get_count(void) {
    return fdi_serial_wire_instrument_count;
}

fdi_serial_wire_instrument_t *fdi_serial_wire_instrument_get_at(uint32_t index) {
    return &fdi_serial_wire_instruments[index];
}

fdi_serial_wire_instrument_t *fdi_serial_wire_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_serial_wire_instrument_count; ++i) {
        fdi_serial_wire_instrument_t *instrument = &fdi_serial_wire_instruments[i];
        if (instrument->super.identifier == identifier) {
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

void fdi_serial_wire_instrument_api_flush(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    if (instrument->tx_data_index > 0) {
        if (fdi_api_send(instrument->super.identifier, apiTypeData, instrument->tx_data, instrument->tx_data_index)) {
            instrument->tx_data_index = 0;
        } else {
            fd_log_assert_fail("can't send");
        }
    }
}

void fdi_serial_wire_instrument_api_set_outputs(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    uint8_t outputs = fd_binary_get_uint8(binary);
    uint8_t values = fd_binary_get_uint8(binary);

    if (outputs & (1 << outputIndicator)) {
        fdi_gpio_set(FDI_GPIO_LED_R, (values & (1 << outputIndicator)) == 0);
    }
    if (outputs & (1 << outputReset)) {
        fdi_serial_wire_set_reset(instrument->serial_wire, (values & (1 << outputReset)) != 0);
    }
    if (outputs & (1 << outputDirection)) {
        if ((values & (1 << outputDirection)) != 0) {
            fdi_serial_wire_set_direction_to_write(instrument->serial_wire);
        } else {
            fdi_serial_wire_set_direction_to_read(instrument->serial_wire);
        }
    }
}

void fdi_serial_wire_instrument_api_get_inputs(uint64_t identifier __attribute__((unused)), uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    uint8_t values = 0;
    if (fdi_serial_wire_get_reset(instrument->serial_wire)) {
        values |= 1 << inputReset;
    }

    uint8_t response_data[16];
    fd_binary_t response;
    fd_binary_initialize(&response, response_data, sizeof(response_data));
    fd_binary_put_varuint(&response, values);
    if (!fdi_api_send(instrument->super.identifier, apiTypeGetInputs, response_data, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_serial_wire_instrument_api_shift_out_bits(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    // bit_count is number of bits - 1
    uint8_t bit_count = fd_binary_get_uint8(binary);
    uint8_t byte = fd_binary_get_uint8(binary);
    fdi_serial_wire_shift_out(instrument->serial_wire, byte, bit_count + 1);
}

void fdi_serial_wire_instrument_api_shift_out_data(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
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

void fdi_serial_wire_instrument_api_shift_in_bits(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    // bit_count is number of bits - 1
    uint8_t bit_count = fd_binary_get_uint8(binary);
    uint8_t byte = fdi_serial_wire_shift_in(instrument->serial_wire, bit_count + 1);
    fdi_serial_wire_instrument_queue_byte(instrument, byte);
}

void fdi_serial_wire_instrument_api_shift_in_data(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
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

void fdi_serial_wire_instrument_api_reset(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_serial_wire_reset(instrument->serial_wire);
    instrument->tx_data_index = 0;
}

void fdi_serial_wire_instrument_api_set_enabled(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    bool enabled = fd_binary_get_uint8(binary) != 0;
    fdi_serial_wire_reset(instrument->serial_wire);
    fdi_serial_wire_set_power(instrument->serial_wire, enabled);
}

void fdi_serial_wire_instrument_api_write_memory(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    uint32_t address = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t length = (uint32_t)fd_binary_get_varuint(binary);
    uint8_t *data = &binary->buffer[binary->get_index];

    fdi_serial_wire_debug_error_t error;
    memset(&error, 0, sizeof(error));
    bool success = fdi_serial_wire_debug_write_data(instrument->serial_wire, address, data, length, &error);

    uint8_t response_data[32];
    fd_binary_t response;
    fd_binary_initialize(&response, response_data, sizeof(response_data));
    fd_binary_put_varuint(&response, success ? 0 : error.code);
    if (!fdi_api_send(instrument->super.identifier, apiTypeWriteMemory, response_data, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_serial_wire_instrument_api_read_memory(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    uint32_t address = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t length = (uint32_t)fd_binary_get_varuint(binary);
    fd_log_assert(length <= 4096);

    uint8_t data[32 + 4096];
    fdi_serial_wire_debug_error_t error;
    memset(&error, 0, sizeof(error));
    bool success = fdi_serial_wire_debug_read_data(instrument->serial_wire, address, &data[1], length, &error);
    data[0] = success ? 0 : error.code;

    if (!fdi_api_send(instrument->super.identifier, apiTypeReadMemory, data, 1 + length)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_serial_wire_instrument_api_write_from_storage(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    uint32_t address = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t length = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t storage_identifier __attribute__((unused)) = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t storage_address = (uint32_t)fd_binary_get_varuint(binary);

    bool success = true;
    fdi_serial_wire_debug_error_t error;
    memset(&error, 0, sizeof(error));
    uint8_t buffer[256];
    for (uint32_t offset = 0; offset < length; offset += sizeof(buffer)) {
        fdi_s25fl116k_read(storage_address + offset, buffer, sizeof(buffer));
        success = fdi_serial_wire_debug_write_data(instrument->serial_wire, address + offset, buffer, sizeof(buffer), &error);
        if (!success) {
            break;
        }
    }

    uint8_t response_data[32];
    fd_binary_t response;
    fd_binary_initialize(&response, response_data, sizeof(response_data));
    fd_binary_put_varuint(&response, success ? 0 : error.code);
    if (!fdi_api_send(instrument->super.identifier, apiTypeWriteFromStorage, response_data, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_serial_wire_instrument_api_compare_memory_to_storage(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    uint32_t address = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t length = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t storage_identifier __attribute__((unused)) = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t storage_address = (uint32_t)fd_binary_get_varuint(binary);

    bool success = true;
    fdi_serial_wire_debug_error_t error;
    memset(&error, 0, sizeof(error));
    uint8_t buffer[256];
    for (uint32_t offset = 0; offset < length; offset += sizeof(buffer)) {
        fdi_s25fl116k_read(storage_address + offset, buffer, sizeof(buffer));
        uint8_t verify[256];
        success = fdi_serial_wire_debug_read_data(instrument->serial_wire, address + offset, verify, sizeof(buffer), &error);
        if (!success) {
            break;
        }
        if (memcmp(buffer, verify, sizeof(buffer)) != 0) {
            success = fdi_serial_wire_debug_error_return(&error, fdi_serial_wire_debug_error_mismatch, 0);
            break;
        }
    }

    uint8_t response_data[32];
    fd_binary_t response;
    fd_binary_initialize(&response, response_data, sizeof(response_data));
    fd_binary_put_varuint(&response, success ? 0 : error.code);
    if (!fdi_api_send(instrument->super.identifier, apiTypeCompareToStorage, response_data, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

#define FDSerialWireDebugTransferTypeReadRegister 0
#define FDSerialWireDebugTransferTypeWriteRegister 1
#define FDSerialWireDebugTransferTypeReadMemory 2
#define FDSerialWireDebugTransferTypeWriteMemory 3

void fdi_serial_wire_instrument_api_transfer(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    uint32_t count = (uint32_t)fd_binary_get_varuint(binary);

    uint8_t response_count = 0;
    uint8_t response_data[512];
    fd_binary_t response;
    fd_binary_initialize(&response, response_data, sizeof(response_data));
    fd_binary_put_varuint(&response, 0 /* assume success for now, replace with error code if an error is encountered */);
    fd_binary_put_varuint(&response, 0 /* placeholder for response count */);

    bool success = true;
    fdi_serial_wire_debug_error_t error;
    memset(&error, 0, sizeof(error));
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t type = (uint32_t)fd_binary_get_varuint(binary);
        switch (type) {
        case FDSerialWireDebugTransferTypeReadRegister: {
            ++response_count;
            fd_binary_put_varuint(&response, type);
            uint16_t register_id = (uint16_t)fd_binary_get_varuint(binary);
            fd_binary_put_varuint(&response, register_id);
            uint32_t value;
            success = fdi_serial_wire_debug_read_register(instrument->serial_wire, register_id, &value, &error);
            fd_binary_put_uint32(&response, value);
        } break;
        case FDSerialWireDebugTransferTypeWriteRegister: {
            uint32_t register_id = (uint16_t)fd_binary_get_varuint(binary);
            uint32_t value = fd_binary_get_uint32(binary);
            success = fdi_serial_wire_debug_write_register(instrument->serial_wire, register_id, value, &error);
        } break;
        case FDSerialWireDebugTransferTypeReadMemory: {
            ++response_count;
            fd_binary_put_varuint(&response, type);
            uint32_t address = fd_binary_get_uint32(binary);
            fd_binary_put_uint32(&response, address);
            uint32_t length = (uint32_t)fd_binary_get_varuint(binary);
            fd_binary_put_varuint(&response, length);
            success = fd_binary_put_check(&response, length);
            if (success) {
                success = fdi_serial_wire_debug_read_data(instrument->serial_wire, address, &response_data[response.put_index], length, &error);
                response.put_index += length;
            }
        } break;
        case FDSerialWireDebugTransferTypeWriteMemory: {
            uint32_t address = fd_binary_get_uint32(binary);
            uint32_t length = (uint32_t)fd_binary_get_varuint(binary);
            success = fd_binary_get_check(binary, length);
            if (success) {
                success = fdi_serial_wire_debug_write_data(instrument->serial_wire, address, &binary->buffer[binary->get_index], length, &error);
                binary->get_index += length;
            }
        } break;
        default:
            fdi_serial_wire_debug_error_return(&error, fdi_serial_wire_debug_error_invalid, 0);
            success = false;
        }
        if (!success) {
            break;
        }
    }
    fd_log_assert(response_count <= 127);
    response_data[1] = response_count;

    if (!success) {
        fd_binary_initialize(&response, response_data, sizeof(response_data));
        fd_binary_put_varuint(&response, error.code);
    }
    if (!fdi_api_send(instrument->super.identifier, apiTypeTransfer, response_data, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_serial_wire_instrument_initialize(void) {
    for (int i = 0; i < fdi_serial_wire_count; ++i) {
        fdi_serial_wire_instrument_t *instrument = &fdi_serial_wire_instruments[i];
        instrument->super.category = "SerialWire";
        instrument->super.reset = fdi_serial_wire_instrument_api_reset;
        instrument->serial_wire = &fdi_serial_wires[i];
        instrument->tx_data_index = 0;
        fdi_instrument_register(&instrument->super);
        uint64_t identifier = instrument->super.identifier;
        fdi_api_register(identifier, apiTypeReset, fdi_serial_wire_instrument_api_reset);
        fdi_api_register(identifier, apiTypeSetOutputs, fdi_serial_wire_instrument_api_set_outputs);
        fdi_api_register(identifier, apiTypeGetInputs, fdi_serial_wire_instrument_api_get_inputs);
        fdi_api_register(identifier, apiTypeShiftOutBits, fdi_serial_wire_instrument_api_shift_out_bits);
        fdi_api_register(identifier, apiTypeShiftOutData, fdi_serial_wire_instrument_api_shift_out_data);
        fdi_api_register(identifier, apiTypeShiftInBits, fdi_serial_wire_instrument_api_shift_in_bits);
        fdi_api_register(identifier, apiTypeShiftInData, fdi_serial_wire_instrument_api_shift_in_data);
        fdi_api_register(identifier, apiTypeFlush, fdi_serial_wire_instrument_api_flush);
        fdi_api_register(identifier, apiTypeSetEnabled, fdi_serial_wire_instrument_api_set_enabled);
        fdi_api_register(identifier, apiTypeWriteMemory, fdi_serial_wire_instrument_api_write_memory);
        fdi_api_register(identifier, apiTypeReadMemory, fdi_serial_wire_instrument_api_read_memory);
        fdi_api_register(identifier, apiTypeWriteFromStorage, fdi_serial_wire_instrument_api_write_from_storage);
        fdi_api_register(identifier, apiTypeCompareToStorage, fdi_serial_wire_instrument_api_compare_memory_to_storage);
        fdi_api_register(identifier, apiTypeTransfer, fdi_serial_wire_instrument_api_transfer);
    }
}
