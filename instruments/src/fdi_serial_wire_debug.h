#ifndef FDI_SERIAL_WIRE_DEBUG_H
#define FDI_SERIAL_WIRE_DEBUG_H

#include "fdi_serial_wire.h"

extern const uint64_t fdi_serial_wire_debug_error_unexpected_ack;
extern const uint64_t fdi_serial_wire_debug_error_too_many_wait_retries;
extern const uint64_t fdi_serial_wire_debug_error_sticky;
extern const uint64_t fdi_serial_wire_debug_error_parity;
extern const uint64_t fdi_serial_wire_debug_error_mismatch;
extern const uint64_t fdi_serial_wire_debug_error_invalid;

typedef struct {
    uint64_t code;
    uint64_t detail;
} fdi_serial_wire_debug_error_t;

bool fdi_serial_wire_debug_error_return(fdi_serial_wire_debug_error_t *error, uint64_t code, uint64_t detail);

#define SWD_MEMORY_CPUID 0xE000ED00
#define SWD_MEMORY_DFSR  0xE000ED30
#define SWD_MEMORY_DHCSR 0xE000EDF0
#define SWD_MEMORY_DCRSR 0xE000EDF4
#define SWD_MEMORY_DCRDR 0xE000EDF8
#define SWD_MEMORY_DEMCR 0xE000EDFC

#define SWD_DHCSR_DBGKEY 0xA05F0000
#define SWD_DHCSR_STAT_RESET_ST  (1 << 25)
#define SWD_DHCSR_STAT_RETIRE_ST (1 << 24)
#define SWD_DHCSR_STAT_LOCKUP    (1 << 19)
#define SWD_DHCSR_STAT_SLEEP     (1 << 18)
#define SWD_DHCSR_STAT_HALT      (1 << 17)
#define SWD_DHCSR_STAT_REGRDY    (1 << 16)
#define SWD_DHCSR_CTRL_SNAPSTALL (1 << 5)
#define SWD_DHCSR_CTRL_MASKINTS  (1 << 3)
#define SWD_DHCSR_CTRL_STEP      (1 << 2)
#define SWD_DHCSR_CTRL_HALT      (1 << 1)
#define SWD_DHCSR_CTRL_DEBUGEN   (1 << 0)

bool fdi_serial_wire_debug_write_data(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    uint8_t *data,
    uint32_t length,
    fdi_serial_wire_debug_error_t *error
);

bool fdi_serial_wire_debug_read_data(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    uint8_t *data,
    uint32_t length,
    fdi_serial_wire_debug_error_t *error
);

bool fdi_serial_wire_debug_read_memory(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    uint32_t *value,
    fdi_serial_wire_debug_error_t *error
);

typedef enum {
    fdi_serial_wire_debug_port_debug,
    fdi_serial_wire_debug_port_access,
} fdi_serial_wire_debug_port_t;

typedef enum {
    fdi_serial_wire_debug_direction_write,
    fdi_serial_wire_debug_direction_read,
} fdi_serial_wire_debug_direction_t;

typedef enum {
    fdi_serial_wire_debug_ack_ok = 0b001,
    fdi_serial_wire_debug_ack_wait = 0b010,
    fdi_serial_wire_debug_ack_fault = 0b100,
} fdi_serial_wire_debug_ack_t;

bool fdi_serial_wire_debug_read_port(
    fdi_serial_wire_t *serial_wire,
    fdi_serial_wire_debug_port_t port,
    uint8_t register_offset,
    uint32_t *value,
    fdi_serial_wire_debug_error_t *error
);

bool fdi_serial_wire_debug_write_port(
    fdi_serial_wire_t *serial_wire,
    fdi_serial_wire_debug_port_t port,
    uint8_t register_offset,
    uint32_t value,
    fdi_serial_wire_debug_error_t *error
);

bool fdi_serial_wire_debug_read_register(
    fdi_serial_wire_t *serial_wire,
    uint16_t register_id,
    uint32_t *value,
    fdi_serial_wire_debug_error_t *error
);

bool fdi_serial_wire_debug_write_register(
    fdi_serial_wire_t *serial_wire,
    uint16_t register_id,
    uint32_t value,
    fdi_serial_wire_debug_error_t *error
);

bool fdi_serial_wire_debug_test(fdi_serial_wire_t *serial_wire);

#endif
