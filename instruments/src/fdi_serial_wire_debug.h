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

#define SWD_DP_DPIDR    0x00
#define SWD_DP_ABORT     0x00
#define SWD_DP_CTRL      0x04
#define SWD_DP_STAT      0x04
#define SWD_DP_SELECT    0x08
#define SWD_DP_RDBUFF    0x0c
#define SWD_DP_TARGETSEL 0x0c
#define SWD_DP_DLCR      0x14
#define SWD_DP_TARGETID  0x24
#define SWD_DP_DLPIDR    0x34
#define SWD_DP_EVENTSTAT 0x44

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

#define SWD_AP_CSW_DBGSWENABLE FDSerialWireDebugBit(31)
#define SWD_AP_CSW_MASTER_DEBUG FDSerialWireDebugBit(29)
#define SWD_AP_CSW_HPROT FDSerialWireDebugBit(25)
#define SWD_AP_CSW_SPIDEN FDSerialWireDebugBit(23)
#define SWD_AP_CSW_TRIN_PROG FDSerialWireDebugBit(7)
#define SWD_AP_CSW_DEVICE_EN FDSerialWireDebugBit(6)
#define SWD_AP_CSW_INCREMENT_PACKED FDSerialWireDebugBit(5)
#define SWD_AP_CSW_INCREMENT_SINGLE FDSerialWireDebugBit(4)
#define SWD_AP_CSW_32BIT FDSerialWireDebugBit(1)
#define SWD_AP_CSW_16BIT FDSerialWireDebugBit(0)

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

// Debug Exception and Monitor Control Register definitions
#define SWD_DEMCR_VC_CORERESET   0x00000001  // Reset Vector Catch
#define SWD_DEMCR_VC_MMERR       0x00000010  // Debug Trap on MMU Fault
#define SWD_DEMCR_VC_NOCPERR     0x00000020  // Debug Trap on No Coprocessor Fault
#define SWD_DEMCR_VC_CHKERR      0x00000040  // Debug Trap on Checking Error Fault
#define SWD_DEMCR_VC_STATERR     0x00000080  // Debug Trap on State Error Fault
#define SWD_DEMCR_VC_BUSERR      0x00000100  // Debug Trap on Bus Error Fault
#define SWD_DEMCR_VC_INTERR      0x00000200  // Debug Trap on Interrupt Error Fault
#define SWD_DEMCR_VC_HARDERR     0x00000400  // Debug Trap on Hard Fault
#define SWD_DEMCR_MON_EN         0x00010000  // Monitor Enable
#define SWD_DEMCR_MON_PEND       0x00020000  // Monitor Pend
#define SWD_DEMCR_MON_STEP       0x00040000  // Monitor Step
#define SWD_DEMCR_MON_REQ        0x00080000  // Monitor Request
#define SWD_DEMCR_TRCENA         0x01000000  // Trace Enable (DWT, ITM, ETM, TPIU)

// NVIC: Application Interrupt/Reset Control Register
#define SWD_MEMORY_NVIC_Addr  0xe000e000
#define SWD_MEMORY_NVIC_AIRCR (SWD_MEMORY_NVIC_Addr + 0x0D0C)
#define SWD_MEMORY_NVIC_AIRCR_VECTRESET      0x00000001  // Reset Cortex-M (except Debug)
#define SWD_MEMORY_NVIC_AIRCR_VECTCLRACTIVE  0x00000002  // Clear Active Vector Bit
#define SWD_MEMORY_NVIC_AIRCR_SYSRESETREQ    0x00000004  // Reset System (except Debug)
#define SWD_MEMORY_NVIC_AIRCR_VECTKEY        0x05FA0000  // Write Key

#define CORTEX_M_REGISTER_R0    0
#define CORTEX_M_REGISTER_R1    1
#define CORTEX_M_REGISTER_R2    2
#define CORTEX_M_REGISTER_R3    3
#define CORTEX_M_REGISTER_R4    4
#define CORTEX_M_REGISTER_R5    5
#define CORTEX_M_REGISTER_R6    6
#define CORTEX_M_REGISTER_R7    7
#define CORTEX_M_REGISTER_R8    8
#define CORTEX_M_REGISTER_R9    9
#define CORTEX_M_REGISTER_R10  10
#define CORTEX_M_REGISTER_R11  11
#define CORTEX_M_REGISTER_R12  12
#define CORTEX_M_REGISTER_IP   12
#define CORTEX_M_REGISTER_R13  13
#define CORTEX_M_REGISTER_SP   13
#define CORTEX_M_REGISTER_R14  14
#define CORTEX_M_REGISTER_LR   14
#define CORTEX_M_REGISTER_R15  15
#define CORTEX_M_REGISTER_PC   15
#define CORTEX_M_REGISTER_XPSR 16
#define CORTEX_M_REGISTER_MSP  17
#define CORTEX_M_REGISTER_PSP  18

#define CORTEX_M_REGISTER_S0  0x40
#define CORTEX_M_REGISTER_S1  0x41
#define CORTEX_M_REGISTER_S2  0x42
#define CORTEX_M_REGISTER_S3  0x43
#define CORTEX_M_REGISTER_S4  0x44
#define CORTEX_M_REGISTER_S5  0x45
#define CORTEX_M_REGISTER_S6  0x46
#define CORTEX_M_REGISTER_S7  0x47
#define CORTEX_M_REGISTER_S8  0x48
#define CORTEX_M_REGISTER_S9  0x49
#define CORTEX_M_REGISTER_S10 0x4a
#define CORTEX_M_REGISTER_S11 0x4b
#define CORTEX_M_REGISTER_S12 0x4c
#define CORTEX_M_REGISTER_S13 0x4d
#define CORTEX_M_REGISTER_S14 0x4e
#define CORTEX_M_REGISTER_S15 0x4f
#define CORTEX_M_REGISTER_S16 0x50
#define CORTEX_M_REGISTER_S17 0x51
#define CORTEX_M_REGISTER_S18 0x52
#define CORTEX_M_REGISTER_S19 0x53
#define CORTEX_M_REGISTER_S20 0x54
#define CORTEX_M_REGISTER_S21 0x55
#define CORTEX_M_REGISTER_S22 0x56
#define CORTEX_M_REGISTER_S23 0x57
#define CORTEX_M_REGISTER_S24 0x58
#define CORTEX_M_REGISTER_S25 0x59
#define CORTEX_M_REGISTER_S26 0x5a
#define CORTEX_M_REGISTER_S27 0x5b
#define CORTEX_M_REGISTER_S28 0x5c
#define CORTEX_M_REGISTER_S29 0x5d
#define CORTEX_M_REGISTER_S30 0x5e
#define CORTEX_M_REGISTER_S31 0x5f

void fdi_serial_wire_debug_set_target_id(
    fdi_serial_wire_t *serial_wire,
    uint32_t target_id
);

void fdi_serial_wire_debug_select_access_port_id(
    fdi_serial_wire_t *serial_wire,
    uint8_t access_port_id
);

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

void fdi_serial_wire_debug_reset_debug_port(
    fdi_serial_wire_t *serial_wire
);

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

bool fdi_serial_wire_debug_initialize_access_port(
    fdi_serial_wire_t *serial_wire,
    fdi_serial_wire_debug_error_t *error
);

bool fdi_serial_wire_debug_is_halted(
    fdi_serial_wire_t *serial_wire,
    bool *halted,
    fdi_serial_wire_debug_error_t *error
);

bool fdi_serial_wire_debug_halt(
    fdi_serial_wire_t *serial_wire,
    fdi_serial_wire_debug_error_t *error
);

bool fdi_serial_wire_debug_step(
    fdi_serial_wire_t *serial_wire,
    fdi_serial_wire_debug_error_t *error
);

bool fdi_serial_wire_debug_run(
    fdi_serial_wire_t *serial_wire,
    fdi_serial_wire_debug_error_t *error
);

#endif
