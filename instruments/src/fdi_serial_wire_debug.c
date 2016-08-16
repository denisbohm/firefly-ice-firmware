#include "fdi_serial_wire_debug.h"

#include "fd_log.h"

#include <stddef.h>

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

#define SWD_DP_IDCODE 0x00
#define SWD_DP_ABORT  0x00
#define SWD_DP_CTRL   0x04
#define SWD_DP_STAT   0x04
#define SWD_DP_SELECT 0x08
#define SWD_DP_RDBUFF 0x0c

#define FDSerialWireDebugBit(n) (1 << (n))

#define SWD_DP_ABORT_ORUNERRCLR FDSerialWireDebugBit(4)
#define SWD_DP_ABORT_WDERRCLR FDSerialWireDebugBit(3)
#define SWD_DP_ABORT_STKERRCLR FDSerialWireDebugBit(2)
#define SWD_DP_ABORT_STKCMPCLR FDSerialWireDebugBit(1)
#define SWD_DP_ABORT_DAPABORT FDSerialWireDebugBit(0)

#define SWD_DP_CTRL_CSYSPWRUPACK FDSerialWireDebugBit(31)
#define SWD_DP_CTRL_CSYSPWRUPREQ FDSerialWireDebugBit(30)
#define SWD_DP_CTRL_CDBGPWRUPACK FDSerialWireDebugBit(29)
#define SWD_DP_CTRL_CDBGPWRUPREQ FDSerialWireDebugBit(28)
#define SWD_DP_CTRL_CDBGRSTACK FDSerialWireDebugBit(27)
#define SWD_DP_CTRL_CDBGRSTREQ FDSerialWireDebugBit(26)
#define SWD_DP_STAT_WDATAERR FDSerialWireDebugBit(7)
#define SWD_DP_STAT_READOK FDSerialWireDebugBit(6)
#define SWD_DP_STAT_STICKYERR FDSerialWireDebugBit(5)
#define SWD_DP_STAT_STICKYCMP FDSerialWireDebugBit(4)
#define SWD_DP_STAT_TRNMODE (FDSerialWireDebugBit(3) | FDSerialWireDebugBit(2))
#define SWD_DP_STAT_STICKYORUN FDSerialWireDebugBit(1)
#define SWD_DP_STAT_ORUNDETECT FDSerialWireDebugBit(0)

#define SWD_AP_CSW 0x00
#define SWD_AP_TAR 0x04
#define SWD_AP_SBZ 0x08
#define SWD_AP_DRW 0x0c
#define SWD_AP_BD0 0x10
#define SWD_AP_BD1 0x14
#define SWD_AP_BD2 0x18
#define SWD_AP_BD3 0x1c
#define SWD_AP_DBGDRAR 0xf8
#define SWD_AP_IDR 0xfc

#define SWD_DP_SELECT_APSEL_APB_AP 0

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
    for (size_t i = 0; i < sizeof(bytes); ++i) {
        fdi_serial_wire_shift_out(serial_wire, bytes[i], 8);
    }
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
    fdi_serial_wire_shift_out(serial_wire, request, 8);
    fdi_serial_wire_set_direction_to_read(serial_wire);
    uint8_t ack = fdi_serial_wire_shift_in(serial_wire, 4) >> 5;
    if (ack != 1) {
        static int errors = 0;
        ++errors;
    }
    return ack;
}

bool fdi_serial_wire_debug_read_uint32(fdi_serial_wire_t *serial_wire, uint32_t *value) {
    uint8_t byte_0 = fdi_serial_wire_shift_in(serial_wire, 8);
    uint8_t byte_1 = fdi_serial_wire_shift_in(serial_wire, 8);
    uint8_t byte_2 = fdi_serial_wire_shift_in(serial_wire, 8);
    uint8_t byte_3 = fdi_serial_wire_shift_in(serial_wire, 8);
    uint8_t parity = fdi_serial_wire_shift_in(serial_wire, 1) >> 7;
    *value = (byte_3 << 24) | (byte_2 << 16) | (byte_1 << 8) | byte_0;
    if (parity != fdi_serial_wire_debug_get_parity_uint32(*value)) {
        return false;
    }
    return true;
}

void fdi_serial_wire_debug_write_uint32(fdi_serial_wire_t *serial_wire, uint32_t value) {
    fdi_serial_wire_shift_out(serial_wire, value, 8);
    fdi_serial_wire_shift_out(serial_wire, value >> 8, 8);
    fdi_serial_wire_shift_out(serial_wire, value >> 16, 8);
    fdi_serial_wire_shift_out(serial_wire, value >> 24, 8);
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

bool fdi_serial_wire_debug_access_port_bank_select(
    fdi_serial_wire_t *serial_wire,
    fdi_serial_wire_debug_port_t access_port,
    uint8_t register_offset,
    fdi_serial_wire_debug_error_t *error
) {
    uint32_t value = (access_port << 24) | (register_offset & 0xf0);
    return fdi_serial_wire_debug_write_port(serial_wire, fdi_serial_wire_debug_port_debug, SWD_DP_SELECT, value, error);
}

bool fdi_serial_wire_debug_select_and_read_access_port(
    fdi_serial_wire_t *serial_wire,
    fdi_serial_wire_debug_port_t access_port,
    uint8_t register_offset,
    uint32_t *value,
    fdi_serial_wire_debug_error_t *error
) {
    if (!fdi_serial_wire_debug_access_port_bank_select(serial_wire, access_port, register_offset, error)) {
        return false;
    }
    uint32_t dummy;
    if (!fdi_serial_wire_debug_read_port(serial_wire, fdi_serial_wire_debug_port_access, register_offset, &dummy, error)) {
        return false;
    }
    if (!fdi_serial_wire_debug_read_port(serial_wire, fdi_serial_wire_debug_port_debug, SWD_DP_RDBUFF, value, error)) {
        return false;
    }
    return true;
}

void fdi_serial_wire_debug_flush(fdi_serial_wire_t *serial_wire) {
    fdi_serial_wire_shift_out(serial_wire, 0, 8);
}

bool fdi_serial_wire_debug_select_and_write_access_port(
    fdi_serial_wire_t *serial_wire,
    fdi_serial_wire_debug_port_t access_port,
    uint8_t register_offset,
    uint32_t value,
    fdi_serial_wire_debug_error_t *error
) {
    if (!fdi_serial_wire_debug_access_port_bank_select(serial_wire, access_port, register_offset, error)) {
        return false;
    }
    if (!fdi_serial_wire_debug_write_port(serial_wire, fdi_serial_wire_debug_port_access, register_offset, value, error)) {
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
    return false;
}

bool fdi_serial_wire_debug_before_memory_transfer(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    fdi_serial_wire_debug_error_t *error
) {
    if (!fdi_serial_wire_debug_select_and_write_access_port(serial_wire, SWD_DP_SELECT_APSEL_APB_AP, SWD_AP_TAR, address, error)) {
        return false;
    }
    if (!fdi_serial_wire_debug_access_port_bank_select(serial_wire, SWD_DP_SELECT_APSEL_APB_AP, SWD_AP_DRW, error)) {
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
    fdi_serial_wire_shift_out(serial_wire, request, 8);
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
    fdi_serial_wire_shift_out(serial_wire, request, 8);
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

bool fdi_serial_wire_debug_write_data(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    uint8_t *data,
    uint32_t length,
    fdi_serial_wire_debug_error_t *error
) {
    if (!fdi_serial_wire_debug_before_memory_transfer(serial_wire, address, error)) {
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

bool fdi_serial_wire_debug_read_data(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    uint8_t *data,
    uint32_t length,
    fdi_serial_wire_debug_error_t *error
) {
    fdi_serial_wire_debug_test(serial_wire);

    if (!fdi_serial_wire_debug_before_memory_transfer(serial_wire, address, error)) {
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
    if (!fdi_serial_wire_debug_read_debug_port(serial_wire, SWD_DP_IDCODE, &dpid, error)) {
        return false;
    }
    uint32_t apid;
    if (!fdi_serial_wire_debug_select_and_read_access_port(serial_wire, SWD_DP_SELECT_APSEL_APB_AP, SWD_AP_IDR, &apid, error)) {
        return false;
    }

    return true;
}

bool fdi_serial_wire_debug_read_memory_uint32(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    uint32_t *value,
    fdi_serial_wire_debug_error_t *error
) {
    if (!fdi_serial_wire_debug_select_and_write_access_port(serial_wire, SWD_DP_SELECT_APSEL_APB_AP, SWD_AP_TAR, address, error)) {
        return false;
    }
    uint32_t tar;
    if (!fdi_serial_wire_debug_select_and_read_access_port(serial_wire, SWD_DP_SELECT_APSEL_APB_AP, SWD_AP_TAR, &tar, error)) {
        return false;
    }
    return fdi_serial_wire_debug_select_and_read_access_port(serial_wire, SWD_DP_SELECT_APSEL_APB_AP, SWD_AP_DRW, value, error);
}

bool fdi_serial_wire_debug_write_memory_uint32(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    uint32_t value,
    fdi_serial_wire_debug_error_t *error
) {
    if (!fdi_serial_wire_debug_select_and_write_access_port(serial_wire, SWD_DP_SELECT_APSEL_APB_AP, SWD_AP_TAR, address, error)) {
        return false;
    }
    return fdi_serial_wire_debug_select_and_write_access_port(serial_wire, SWD_DP_SELECT_APSEL_APB_AP, SWD_AP_DRW, value, error);
}

void fdi_serial_wire_debug_test(fdi_serial_wire_t *serial_wire) {
    fdi_serial_wire_debug_error_t error;
    bool result;

    fdi_serial_wire_debug_reset_debug_port(serial_wire);

    uint32_t dpid;
    result = fdi_serial_wire_debug_read_debug_port(serial_wire, SWD_DP_IDCODE, &dpid, &error);

    result = fdi_serial_wire_debug_initialize_debug_port(serial_wire, &error);

    uint32_t address = 0x20000000;
    uint32_t value = 0;
    result = fdi_serial_wire_debug_read_memory_uint32(serial_wire, address, &value, &error);
    result = fdi_serial_wire_debug_write_memory_uint32(serial_wire, address, 0x12345678, &error);
    result = fdi_serial_wire_debug_read_memory_uint32(serial_wire, address, &value, &error);
}