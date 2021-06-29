#include "fdi_serial_wire_debug.h"

#include "fd_log.h"

#include <stddef.h>
#include <string.h>

bool fdi_serial_wire_debug_error_return(fdi_serial_wire_debug_error_t *error, uint64_t code, uint64_t detail) {
    if (error) {
        error->code = code;
        error->detail = detail;
    }
    return false;
}

const uint64_t fdi_serial_wire_debug_error_unexpected_ack = 1;
const uint64_t fdi_serial_wire_debug_error_too_many_wait_retries = 2;
const uint64_t fdi_serial_wire_debug_error_sticky = 3;
const uint64_t fdi_serial_wire_debug_error_parity = 4;
const uint64_t fdi_serial_wire_debug_error_mismatch = 5;
const uint64_t fdi_serial_wire_debug_error_invalid = 6;
const uint64_t fdi_serial_wire_debug_error_not_ready = 7;

void fdi_serial_wire_debug_reset_debug_port(fdi_serial_wire_t *serial_wire) {
    fdi_serial_wire_set_direction_to_write(serial_wire);
    uint8_t bytes[] = {
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0x9e,
        0xe7,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0xff,
        0x00,
    };
    fdi_serial_wire_shift_out_bytes(serial_wire, bytes, sizeof(bytes));
}

void fdi_serial_wire_debug_flush(fdi_serial_wire_t *serial_wire) {
    uint8_t data = 0;
    fdi_serial_wire_shift_out_bytes(serial_wire, &data, 1);
}

uint8_t fdi_serial_wire_debug_get_parity_uint8(uint8_t v) {
    return (0x6996 >> ((v ^ (v >> 4)) & 0xf)) & 1;
}

uint8_t fdi_serial_wire_debug_get_parity_uint32(uint32_t v) {
    v ^= v >> 16;
    v ^= v >> 8;
    return fdi_serial_wire_debug_get_parity_uint8(v);
}

uint8_t fdi_serial_wire_debug_encode_request(fdi_serial_wire_debug_port_t port, fdi_serial_wire_debug_direction_t direction, uint8_t address) {
    uint8_t request = 0b10000001; // Start (bit 0) & Park (bit 7)
    if (port == fdi_serial_wire_debug_port_access) {
        request |= 0b00000010;
    }
    if (direction == fdi_serial_wire_debug_direction_read) {
        request |= 0b00000100;
    }
    request |= (address << 1) & 0b00011000;
    if (fdi_serial_wire_debug_get_parity_uint8(request)) {
        request |= 0b00100000;
    }
    return request;
}

fdi_serial_wire_debug_ack_t fdi_serial_wire_debug_request(fdi_serial_wire_t *serial_wire, uint8_t request) {
    fdi_serial_wire_shift_out_bytes(serial_wire, &request, 1);
    fdi_serial_wire_set_direction_to_read(serial_wire);
    uint8_t ack = fdi_serial_wire_shift_in(serial_wire, 4) >> 5;
    if (ack != 1) {
        static int errors = 0;
        ++errors;
    }
    return ack;
}

bool fdi_serial_wire_debug_read_uint32(fdi_serial_wire_t *serial_wire, uint32_t *value) {
    if (serial_wire->half_bit_delay_ns == 0) {
        fdi_serial_wire_shift_in_bytes(serial_wire, (uint8_t *)value, sizeof(uint32_t));
    } else {
        uint32_t byte_0 = fdi_serial_wire_shift_in(serial_wire, 8);
        uint32_t byte_1 = fdi_serial_wire_shift_in(serial_wire, 8);
        uint32_t byte_2 = fdi_serial_wire_shift_in(serial_wire, 8);
        uint32_t byte_3 = fdi_serial_wire_shift_in(serial_wire, 8);
        *value = (byte_3 << 24) | (byte_2 << 16) | (byte_1 << 8) | byte_0;
    }
    uint8_t parity = fdi_serial_wire_shift_in(serial_wire, 1) >> 7;
    if (parity != fdi_serial_wire_debug_get_parity_uint32(*value)) {
        return false;
    }
    return true;
}

void fdi_serial_wire_debug_write_uint32(fdi_serial_wire_t *serial_wire, uint32_t value) {
    if (serial_wire->half_bit_delay_ns == 0) {
        fdi_serial_wire_shift_out_bytes(serial_wire, (uint8_t *)&value, sizeof(uint32_t));
    } else {
        fdi_serial_wire_shift_out(serial_wire, value, 8);
        fdi_serial_wire_shift_out(serial_wire, value >> 8, 8);
        fdi_serial_wire_shift_out(serial_wire, value >> 16, 8);
        fdi_serial_wire_shift_out(serial_wire, value >> 24, 8);
    }
    uint8_t parity = fdi_serial_wire_debug_get_parity_uint32(value);
    fdi_serial_wire_shift_out(serial_wire, parity, 1);
}

void fdi_serial_wire_debug_turn_to_write_and_skip(fdi_serial_wire_t *serial_wire) {
    fdi_serial_wire_shift_in(serial_wire, 1);
    fdi_serial_wire_set_direction_to_write(serial_wire);
}

bool fdi_serial_wire_debug_read_port(
    fdi_serial_wire_t *serial_wire,
    fdi_serial_wire_debug_port_t port,
    uint8_t register_offset,
    uint32_t *value,
    fdi_serial_wire_debug_error_t *error
) {
    bool overrun_detection_enabled = serial_wire->overrun_detection_enabled;
    uint32_t retry_count = serial_wire->ack_wait_retry_count;

    fdi_serial_wire_debug_ack_t ack = fdi_serial_wire_debug_ack_ok;
    uint8_t request = fdi_serial_wire_debug_encode_request(port, fdi_serial_wire_debug_direction_read, register_offset);
    for (uint32_t retry = 0; retry < retry_count; ++retry) {
        ack = fdi_serial_wire_debug_request(serial_wire, request);
        if (overrun_detection_enabled) {
            if (!fdi_serial_wire_debug_read_uint32(serial_wire, value)) {
                continue;
            }
            fdi_serial_wire_debug_turn_to_write_and_skip(serial_wire);
        }
        if (ack == fdi_serial_wire_debug_ack_ok) {
            if (!overrun_detection_enabled) {
                if (!fdi_serial_wire_debug_read_uint32(serial_wire, value)) {
                    continue;
                }
                fdi_serial_wire_debug_turn_to_write_and_skip(serial_wire);
            }
            return true;
        }
        if (!overrun_detection_enabled) {
            fdi_serial_wire_debug_turn_to_write_and_skip(serial_wire);
        }
        if (ack != fdi_serial_wire_debug_ack_wait) {
            return fdi_serial_wire_debug_error_return(error, fdi_serial_wire_debug_error_unexpected_ack, ack);
        }
    }
    fd_log_assert(ack == fdi_serial_wire_debug_ack_wait);
    return fdi_serial_wire_debug_error_return(error, fdi_serial_wire_debug_error_too_many_wait_retries, ack);
}

bool fdi_serial_wire_debug_write_port(
    fdi_serial_wire_t *serial_wire,
    fdi_serial_wire_debug_port_t port,
    uint8_t register_offset,
    uint32_t value,
    fdi_serial_wire_debug_error_t *error
) {
    bool overrun_detection_enabled = serial_wire->overrun_detection_enabled;
    uint32_t retry_count = serial_wire->ack_wait_retry_count;

    fdi_serial_wire_debug_ack_t ack = fdi_serial_wire_debug_ack_ok;
    uint8_t request = fdi_serial_wire_debug_encode_request(port, fdi_serial_wire_debug_direction_write, register_offset);
    for (uint32_t retry = 0; retry < retry_count; ++retry) {
        ack = fdi_serial_wire_debug_request(serial_wire, request);
        fdi_serial_wire_debug_turn_to_write_and_skip(serial_wire);
        if (overrun_detection_enabled) {
            fdi_serial_wire_debug_write_uint32(serial_wire, value);
        }
        if (ack == fdi_serial_wire_debug_ack_ok) {
            if (!overrun_detection_enabled) {
                fdi_serial_wire_debug_write_uint32(serial_wire, value);
            }
            return true;
        }
        if (ack != fdi_serial_wire_debug_ack_wait) {
            return fdi_serial_wire_debug_error_return(error, fdi_serial_wire_debug_error_unexpected_ack, ack);
        }
    }
    fd_log_assert(ack == fdi_serial_wire_debug_ack_wait);
    return fdi_serial_wire_debug_error_return(error, fdi_serial_wire_debug_error_too_many_wait_retries, ack);
}

bool fdi_serial_wire_debug_read_debug_port(
    fdi_serial_wire_t *serial_wire,
    uint8_t register_offset,
    uint32_t *value,
    fdi_serial_wire_debug_error_t *error
) {
    return fdi_serial_wire_debug_read_port(serial_wire, fdi_serial_wire_debug_port_debug, register_offset, value, error);
}

bool fdi_serial_wire_debug_write_debug_port(
    fdi_serial_wire_t *serial_wire,
    uint8_t register_offset,
    uint32_t value,
    fdi_serial_wire_debug_error_t *error
) {
    return fdi_serial_wire_debug_write_port(serial_wire, fdi_serial_wire_debug_port_debug, register_offset, value, error);
}

bool fdi_serial_wire_debug_read_access_port(
    fdi_serial_wire_t *serial_wire,
    uint8_t register_offset,
    uint32_t *value,
    fdi_serial_wire_debug_error_t *error
) {
    return fdi_serial_wire_debug_read_port(serial_wire, fdi_serial_wire_debug_port_access, register_offset, value, error);
}

bool fdi_serial_wire_debug_write_access_port(
    fdi_serial_wire_t *serial_wire,
    uint8_t register_offset,
    uint32_t value,
    fdi_serial_wire_debug_error_t *error
) {
    return fdi_serial_wire_debug_write_port(serial_wire, fdi_serial_wire_debug_port_access, register_offset, value, error);
}

void fdi_serial_wire_debug_set_target_id(fdi_serial_wire_t *serial_wire, uint32_t target_id) {
    serial_wire->target_id = target_id;
}

void fdi_serial_wire_debug_select_access_port_id(fdi_serial_wire_t *serial_wire, uint8_t access_port_id) {
    serial_wire->debug_port_access.fields.access_port_id = access_port_id;
}

void fdi_serial_wire_debug_select_access_port_bank_select(fdi_serial_wire_t *serial_wire, uint8_t access_port_bank_select) {
    serial_wire->debug_port_access.fields.access_port_bank_select = access_port_bank_select;
}

void fdi_serial_wire_debug_select_access_port_register(fdi_serial_wire_t *serial_wire, uint8_t access_port_register) {
    fdi_serial_wire_debug_select_access_port_bank_select(serial_wire, (access_port_register >> 4) & 0xf);
}

void fdi_serial_wire_debug_select_debug_port_bank_select(fdi_serial_wire_t *serial_wire, uint8_t debug_port_bank_select) {
    serial_wire->debug_port_access.fields.debug_port_bank_select = debug_port_bank_select;
}

void fdi_serial_wire_debug_select_debug_port_register(fdi_serial_wire_t *serial_wire, uint8_t debug_port_register) {
    fdi_serial_wire_debug_select_debug_port_bank_select(serial_wire, (debug_port_register >> 4) & 0xf);
}

bool fdi_serial_wire_debug_port_select(fdi_serial_wire_t *serial_wire, fdi_serial_wire_debug_error_t *error) {
    uint32_t value = serial_wire->debug_port_access.value;
    return fdi_serial_wire_debug_write_port(serial_wire, fdi_serial_wire_debug_port_debug, SWD_DP_SELECT, value, error);
}

bool fdi_serial_wire_debug_select_and_read_access_port(
    fdi_serial_wire_t *serial_wire,
    uint8_t access_port_register,
    uint32_t *value,
    fdi_serial_wire_debug_error_t *error
) {
    fdi_serial_wire_debug_select_access_port_register(serial_wire, access_port_register);
    if (!fdi_serial_wire_debug_port_select(serial_wire, error)) {
        return false;
    }
    uint32_t dummy;
    if (!fdi_serial_wire_debug_read_port(serial_wire, fdi_serial_wire_debug_port_access, access_port_register, &dummy, error)) {
        return false;
    }
    if (!fdi_serial_wire_debug_read_port(serial_wire, fdi_serial_wire_debug_port_debug, SWD_DP_RDBUFF, value, error)) {
        return false;
    }
    return true;
}

bool fdi_serial_wire_debug_select_and_write_access_port(
    fdi_serial_wire_t *serial_wire,
    uint8_t access_port_register,
    uint32_t value,
    fdi_serial_wire_debug_error_t *error
) {
    fdi_serial_wire_debug_select_access_port_register(serial_wire, access_port_register);
    if (!fdi_serial_wire_debug_port_select(serial_wire, error)) {
        return false;
    }
    if (!fdi_serial_wire_debug_write_port(serial_wire, fdi_serial_wire_debug_port_access, access_port_register, value, error)) {
        return false;
    }
    fdi_serial_wire_debug_flush(serial_wire);
    return true;
}

bool fdi_serial_wire_debug_set_overrun_detection(
    fdi_serial_wire_t *serial_wire,
    bool enabled,
    fdi_serial_wire_debug_error_t *error
) {
    const uint32_t flags =
        SWD_DP_ABORT_ORUNERRCLR |
        SWD_DP_ABORT_WDERRCLR |
        SWD_DP_ABORT_STKERRCLR |
        SWD_DP_ABORT_STKCMPCLR;
    if (!fdi_serial_wire_debug_write_debug_port(serial_wire, SWD_DP_ABORT, flags, error)) {
        return false;
    }
    
    uint32_t value = SWD_DP_CTRL_CDBGPWRUPREQ | SWD_DP_CTRL_CSYSPWRUPREQ;
    if (enabled) {
        value |= SWD_DP_STAT_ORUNDETECT;
    }
    if (!fdi_serial_wire_debug_write_debug_port(serial_wire, SWD_DP_CTRL, value, error)) {
        return false;
    }
    
    serial_wire->overrun_detection_enabled = enabled;
    return true;
}

#define tar_increment_bits 0x3f

bool fdi_serial_wire_debug_before_memory_transfer(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    uint32_t length,
    fdi_serial_wire_debug_error_t *error
) {
    if ((address & 0x3) != 0) {
        return fdi_serial_wire_debug_error_return(error, fdi_serial_wire_debug_error_invalid, address);
    }
    if ((length == 0) || ((length & 0x3) != 0)) {
        return fdi_serial_wire_debug_error_return(error, fdi_serial_wire_debug_error_invalid, length);
    }
    // TAR auto increment is only guaranteed in the first 10-bits (beyond that is implementation defined)
    uint32_t end_address = address + length - 1;
    if ((address & ~tar_increment_bits) != (end_address & ~tar_increment_bits)) {
        return fdi_serial_wire_debug_error_return(error, fdi_serial_wire_debug_error_invalid, length);
    }

    if (!fdi_serial_wire_debug_select_and_write_access_port(serial_wire, SWD_AP_TAR, address, error)) {
        return false;
    }
    fdi_serial_wire_debug_select_access_port_register(serial_wire, SWD_AP_DRW);
    if (!fdi_serial_wire_debug_port_select(serial_wire, error)) {
        return false;
    }
    return fdi_serial_wire_debug_set_overrun_detection(serial_wire, true, error);
}

bool fdi_serial_wire_debug_after_memory_transfer(
    fdi_serial_wire_t *serial_wire,
    fdi_serial_wire_debug_error_t *error
) {
    uint32_t status;
    if (!fdi_serial_wire_debug_read_debug_port(serial_wire, SWD_DP_STAT, &status, error)) {
        return false;
    }
    if (!fdi_serial_wire_debug_set_overrun_detection(serial_wire, false, error)) {
        return false;
    }
    if (status & (SWD_DP_STAT_WDATAERR | SWD_DP_STAT_STICKYERR | SWD_DP_STAT_STICKYORUN)) {
        return fdi_serial_wire_debug_error_return(error, fdi_serial_wire_debug_error_sticky, status);
    }
    return true;
}

bool fdi_serial_wire_debug_request_write(fdi_serial_wire_t *serial_wire, uint8_t request, uint32_t value, fdi_serial_wire_debug_error_t *error) {
    fdi_serial_wire_shift_out_bytes(serial_wire, &request, 1);
    fdi_serial_wire_set_direction_to_read(serial_wire);
    fdi_serial_wire_debug_ack_t ack = fdi_serial_wire_shift_in(serial_wire, 4) >> 5;
    // !!! is this the right error handling?  to bail at this point or finish the sequence below? -denis
    if (ack != fdi_serial_wire_debug_ack_ok) {
        return fdi_serial_wire_debug_error_return(error, fdi_serial_wire_debug_error_unexpected_ack, ack);
    }
    fdi_serial_wire_debug_turn_to_write_and_skip(serial_wire);
    fdi_serial_wire_debug_write_uint32(serial_wire, value);
    return true;
}

bool fdi_serial_wire_debug_request_read(fdi_serial_wire_t *serial_wire, uint8_t request, uint32_t *value, fdi_serial_wire_debug_error_t *error) {
    fdi_serial_wire_shift_out_bytes(serial_wire, &request, 1);
    fdi_serial_wire_set_direction_to_read(serial_wire);
    fdi_serial_wire_debug_ack_t ack = fdi_serial_wire_shift_in(serial_wire, 4) >> 5;
    // !!! is this the right error handling?  to bail at this point or finish the sequence below? -denis
    if (ack != fdi_serial_wire_debug_ack_ok) {
        return fdi_serial_wire_debug_error_return(error, fdi_serial_wire_debug_error_unexpected_ack, ack);
    }
    if (!fdi_serial_wire_debug_read_uint32(serial_wire, value)) {
        return fdi_serial_wire_debug_error_return(error, fdi_serial_wire_debug_error_parity, *value);
    }
    fdi_serial_wire_debug_turn_to_write_and_skip(serial_wire);
    return true;
}

static uint32_t unpack_little_endian_uint32(uint8_t *bytes) {
    return (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
}

static void pack_little_endian_uint32(uint8_t *bytes, uint32_t value) {
    bytes[0] = value;
    bytes[1] = value >> 8;
    bytes[2] = value >> 16;
    bytes[3] = value >> 24;
}

bool fdi_serial_wire_debug_write_memory_transfer(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    uint8_t *data,
    uint32_t length,
    fdi_serial_wire_debug_error_t *error
) {
    if (!fdi_serial_wire_debug_before_memory_transfer(serial_wire, address, length, error)) {
        return false;
    }
    
    bool success = true;
    uint8_t request = fdi_serial_wire_debug_encode_request(fdi_serial_wire_debug_port_access, fdi_serial_wire_debug_direction_write, SWD_AP_DRW);
    for (uint32_t i = 0; i < length; i += 4) {
        if (!fdi_serial_wire_debug_request_write(serial_wire, request, unpack_little_endian_uint32(&data[i]), error)) {
            success = false;
            break;
        }
    }
    
    if (!fdi_serial_wire_debug_after_memory_transfer(serial_wire, error)) {
        return false;
    }

    return success;
}

bool fdi_serial_wire_debug_read_memory_transfer(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    uint8_t *data,
    uint32_t length,
    fdi_serial_wire_debug_error_t *error
) {
    if (!fdi_serial_wire_debug_before_memory_transfer(serial_wire, address, length, error)) {
        return false;
    }
    
    uint8_t request = fdi_serial_wire_debug_encode_request(fdi_serial_wire_debug_port_access, fdi_serial_wire_debug_direction_read, SWD_AP_DRW);
    uint32_t words = length / 4;
    uint32_t index = 0;
    bool success = true;
    // note: 1 extra iteration because of 1 read delay in getting data out
    uint32_t value;
    if (!fdi_serial_wire_debug_request_read(serial_wire, request, &value, error)) {
        success = false;
    } else {
        for (uint32_t i = 0; i < words; ++i) {
            if (!fdi_serial_wire_debug_request_read(serial_wire, request, &value, error)) {
                success = false;
                break;
            }
            pack_little_endian_uint32(&data[index], value);
            index += 4;
        }
    }
    
    if (!fdi_serial_wire_debug_after_memory_transfer(serial_wire, error)) {
        return false;
    }

    return success;
}

bool fdi_serial_wire_debug_write_data(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    uint8_t *data,
    uint32_t length,
    fdi_serial_wire_debug_error_t *error
) {
    uint32_t offset = 0;
    while (length > 0) {
        uint32_t sublength = (tar_increment_bits + 1) - (address & tar_increment_bits);
        if (length < sublength) {
            sublength = length;
        }
        
        if (!fdi_serial_wire_debug_write_memory_transfer(serial_wire, address, data, sublength, error)) {
            return false;
        }
        
        address += sublength;
        data += sublength;
        length -= sublength;
        offset += sublength;
    }
    return true;
}

bool fdi_serial_wire_debug_read_data(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    uint8_t *data,
    uint32_t length,
    fdi_serial_wire_debug_error_t *error
) {
    uint32_t offset = 0;
    while (length > 0) {
        uint32_t sublength = (tar_increment_bits + 1) - (address & tar_increment_bits);
        if (length < sublength) {
            sublength = length;
        }
        
        if (!fdi_serial_wire_debug_read_memory_transfer(serial_wire, address, data, sublength, error)) {
            return false;
        }
        
        address += sublength;
        data += sublength;
        length -= sublength;
        offset += sublength;
    }
    return true;
}

#define SWD_DP_SELECT_APSEL_NRF52_CTRL_AP 1

#define NRF52_AP_REG_RESET 0x00
#define NRF52_AP_REG_ERASEALL 0x04
#define NRF52_AP_REG_ERASEALLSTATUS 0x08
#define NRF52_AP_REG_APPROTECTSTATUS 0x0c
#define NRF52_AP_REG_IDR 0xfc

#define NRF52_CTRL_AP_ID 0x02880000

bool fdi_serial_wire_debug_wait_for_debug_port_status(
    fdi_serial_wire_t *serial_wire,
    uint32_t mask,
    fdi_serial_wire_debug_error_t *error
) {
    int debug_port_status_retry_count = 3;
    uint32_t status = 0;
    for (int retry = 0; retry < debug_port_status_retry_count; ++retry) {
        if (!fdi_serial_wire_debug_read_debug_port(serial_wire, SWD_DP_STAT, &status, error)) {
            return false;
        }
        if (status & mask) {
            return true;
        }
    }
    return fdi_serial_wire_debug_error_return(error, fdi_serial_wire_debug_error_too_many_wait_retries, status);
}

bool fdi_serial_wire_debug_initialize_debug_port(
    fdi_serial_wire_t *serial_wire,
    fdi_serial_wire_debug_error_t *error
) {
    uint32_t stat;
    if (!fdi_serial_wire_debug_read_debug_port(serial_wire, SWD_DP_STAT, &stat, error)) {
        return false;
    }
    
    if (!fdi_serial_wire_debug_write_debug_port(serial_wire, SWD_DP_ABORT,
                              SWD_DP_ABORT_ORUNERRCLR |
                              SWD_DP_ABORT_WDERRCLR |
                              SWD_DP_ABORT_STKERRCLR |
                              SWD_DP_ABORT_STKCMPCLR, error)
    ) {
        return false;
    }

    if (!fdi_serial_wire_debug_read_debug_port(serial_wire, SWD_DP_STAT, &stat, error)) {
        return false;
    }

    if (!fdi_serial_wire_debug_write_debug_port(serial_wire, SWD_DP_CTRL, SWD_DP_CTRL_CDBGPWRUPREQ | SWD_DP_CTRL_CSYSPWRUPREQ, error)) {
        return false;
    }

    if (!fdi_serial_wire_debug_wait_for_debug_port_status(serial_wire, SWD_DP_CTRL_CSYSPWRUPACK, error)) {
        return false;
    }

    if (!fdi_serial_wire_debug_wait_for_debug_port_status(serial_wire, SWD_DP_CTRL_CDBGPWRUPACK, error)) {
        return false;
    }

    if (!fdi_serial_wire_debug_write_debug_port(serial_wire, SWD_DP_SELECT, 0, error)) {
        return false;
    }

    if (!fdi_serial_wire_debug_read_debug_port(serial_wire, SWD_DP_STAT, &stat, error)) {
        return false;
    }

    // cache values needed for various higher level routines (such as reading and writing to memory in bulk)
    uint32_t dpid;
    if (!fdi_serial_wire_debug_read_debug_port(serial_wire, SWD_DP_DPIDR, &dpid, error)) {
        return false;
    }
    uint32_t apid;
    if (!fdi_serial_wire_debug_select_and_read_access_port(serial_wire, SWD_AP_IDR, &apid, error)) {
        return false;
    }

    return true;
}

bool fdi_serial_wire_debug_initialize_access_port(fdi_serial_wire_t *serial_wire, fdi_serial_wire_debug_error_t *error) {
  return fdi_serial_wire_debug_select_and_write_access_port(
      serial_wire,
      SWD_AP_CSW,
      SWD_AP_CSW_DBGSWENABLE | SWD_AP_CSW_MASTER_DEBUG | SWD_AP_CSW_HPROT | SWD_AP_CSW_INCREMENT_SINGLE | SWD_AP_CSW_32BIT,
      error);
}

bool fdi_serial_wire_debug_read_memory_uint32(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    uint32_t *value,
    fdi_serial_wire_debug_error_t *error
) {
    if (!fdi_serial_wire_debug_select_and_write_access_port(serial_wire, SWD_AP_TAR, address, error)) {
        return false;
    }
    uint32_t tar;
    if (!fdi_serial_wire_debug_select_and_read_access_port(serial_wire, SWD_AP_TAR, &tar, error)) {
        return false;
    }
    return fdi_serial_wire_debug_select_and_read_access_port(serial_wire, SWD_AP_DRW, value, error);
}

bool fdi_serial_wire_debug_write_memory_uint32(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    uint32_t value,
    fdi_serial_wire_debug_error_t *error
) {
    if (!fdi_serial_wire_debug_select_and_write_access_port(serial_wire, SWD_AP_TAR, address, error)) {
        return false;
    }
    return fdi_serial_wire_debug_select_and_write_access_port(serial_wire, SWD_AP_DRW, value, error);
}

bool fdi_serial_wire_debug_wait_for_register_ready(
    fdi_serial_wire_t *serial_wire,
    fdi_serial_wire_debug_error_t *error
) {
    uint32_t retry_count = serial_wire->register_retry_count;
    for (uint32_t retry = 0; retry < retry_count; ++retry) {
        uint32_t dhscr;
        if (!fdi_serial_wire_debug_read_memory_uint32(serial_wire, SWD_MEMORY_DHCSR, &dhscr, error)) {
            return false;
        }
        if (dhscr & SWD_DHCSR_STAT_REGRDY) {
            return true;
        }
    }
    return fdi_serial_wire_debug_error_return(error, fdi_serial_wire_debug_error_not_ready, 0);
}

bool fdi_serial_wire_debug_read_register(
    fdi_serial_wire_t *serial_wire,
    uint16_t register_id,
    uint32_t *value,
    fdi_serial_wire_debug_error_t *error
) {
    if (!fdi_serial_wire_debug_write_memory_uint32(serial_wire, SWD_MEMORY_DCRSR, register_id, error)) {
        return false;
    }
    if (!fdi_serial_wire_debug_wait_for_register_ready(serial_wire, error)) {
        return false;
    }
    if (!fdi_serial_wire_debug_read_memory_uint32(serial_wire, SWD_MEMORY_DCRDR, value, error)) {
        return false;
    }
    return true;
}

bool fdi_serial_wire_debug_write_register(
    fdi_serial_wire_t *serial_wire,
    uint16_t register_id,
    uint32_t value,
    fdi_serial_wire_debug_error_t *error
) {
    if (!fdi_serial_wire_debug_write_memory_uint32(serial_wire, SWD_MEMORY_DCRDR, value, error)) {
        return false;
    }
    if (!fdi_serial_wire_debug_write_memory_uint32(serial_wire, SWD_MEMORY_DCRSR, 0x00010000 | register_id, error)) {
        return false;
    }
    if (!fdi_serial_wire_debug_wait_for_register_ready(serial_wire, error)) {
        return false;
    }
    return true;
}

bool fdi_serial_wire_debug_is_halted(fdi_serial_wire_t *serial_wire, bool *halted, fdi_serial_wire_debug_error_t *error) {
    uint32_t dhcsr;
    if (!fdi_serial_wire_debug_read_memory_uint32(serial_wire, SWD_MEMORY_DHCSR, &dhcsr, error)) {
        return false;
    }
    *halted = (dhcsr & SWD_DHCSR_STAT_HALT) != 0;
    return true;
}

bool fdi_serial_wire_debug_write_DHCSR(fdi_serial_wire_t *serial_wire, uint32_t value, fdi_serial_wire_debug_error_t *error) {
    value |= SWD_DHCSR_DBGKEY | SWD_DHCSR_CTRL_DEBUGEN;
    return fdi_serial_wire_debug_write_memory_uint32(serial_wire, SWD_MEMORY_DHCSR, value, error);
}

bool fdi_serial_wire_debug_halt(fdi_serial_wire_t *serial_wire, fdi_serial_wire_debug_error_t *error) {
    return fdi_serial_wire_debug_write_DHCSR(serial_wire, SWD_DHCSR_CTRL_HALT, error);
}

bool fdi_serial_wire_debug_step(fdi_serial_wire_t *serial_wire, fdi_serial_wire_debug_error_t *error) {
    return fdi_serial_wire_debug_write_DHCSR(serial_wire, SWD_DHCSR_CTRL_STEP, error);
}

bool fdi_serial_wire_debug_run(fdi_serial_wire_t *serial_wire, fdi_serial_wire_debug_error_t *error) {
    return fdi_serial_wire_debug_write_DHCSR(serial_wire, 0, error);
}

bool fdi_serial_wire_debug_connect(
    fdi_serial_wire_t *serial_wire,
    uint32_t *dpid,
    fdi_serial_wire_debug_error_t *error
) {
    fdi_serial_wire_debug_reset_debug_port(serial_wire);

    if (serial_wire->target_id != 0) {
        fdi_serial_wire_debug_error_t error;
        memset(&error, 0, sizeof(error));
        uint32_t ack_wait_retry_count = serial_wire->ack_wait_retry_count;
        serial_wire->ack_wait_retry_count = 1;
        fdi_serial_wire_debug_write_debug_port(serial_wire, SWD_DP_TARGETSEL, serial_wire->target_id, &error);
        serial_wire->ack_wait_retry_count = ack_wait_retry_count;
    }

    bool success = fdi_serial_wire_debug_read_debug_port(serial_wire, SWD_DP_DPIDR, dpid, error);

    if (success) {
        success = fdi_serial_wire_debug_initialize_debug_port(serial_wire, error);
    }

    if (success) {
        success = fdi_serial_wire_debug_initialize_access_port(serial_wire, error);
    }

    if (success) {
        success = fdi_serial_wire_debug_halt(serial_wire, error);
    }

    return success;
}