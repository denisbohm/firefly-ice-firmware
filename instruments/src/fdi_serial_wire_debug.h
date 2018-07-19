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

bool fdi_serial_wire_debug_read_memory_uint32(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    uint32_t *value,
    fdi_serial_wire_debug_error_t *error
);

bool fdi_serial_wire_debug_write_memory_uint32(
    fdi_serial_wire_t *serial_wire,
    uint32_t address,
    uint32_t value,
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

void fdi_serial_wire_debug_reset_debug_port(fdi_serial_wire_t *serial_wire);

bool fdi_serial_wire_debug_initialize_debug_port(
    fdi_serial_wire_t *serial_wire,
    fdi_serial_wire_debug_error_t *error
);

bool fdi_serial_wire_debug_read_debug_port(
    fdi_serial_wire_t *serial_wire,
    uint8_t register_offset,
    uint32_t *value,
    fdi_serial_wire_debug_error_t *error
);

bool fdi_serial_wire_debug_write_debug_port(
    fdi_serial_wire_t *serial_wire,
    uint8_t register_offset,
    uint32_t value,
    fdi_serial_wire_debug_error_t *error
);

bool fdi_serial_wire_debug_test(fdi_serial_wire_t *serial_wire);

#endif
