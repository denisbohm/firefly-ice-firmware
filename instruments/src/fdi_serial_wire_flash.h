#ifndef FDI_SERIAL_WIRE_FLASH_H
#define FDI_SERIAL_WIRE_FLASH_H

#include "fdi_firefly_flash.h"
#include "fdi_serial_wire_debug.h"

#include "fd_range.h"

typedef struct {
    fd_range_t ram;
    fd_range_t stack;
    fd_range_t heap;
    uint32_t halt_address;
} fdi_serial_wire_flash_rpc_t;

typedef void (*fdi_serial_wire_flash_reader_t)(void *context, uint32_t address, uint32_t length, uint8_t *data);

typedef struct {
    fdi_serial_wire_t *serial_wire;
    fdi_serial_wire_flash_rpc_t rpc;
    fdi_firefly_flash_t *firefly_flash;
    uint32_t address;
    uint32_t length;
    fdi_serial_wire_flash_reader_t reader;
    uint32_t reader_address;
} fdi_serial_wire_flash_t;

bool fdi_serial_wire_flash(fdi_serial_wire_flash_t *flash, fdi_serial_wire_debug_error_t *error);

#endif
