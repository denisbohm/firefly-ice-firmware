#include "fdi_apollo2.h"

#include "fdi_delay.h"

bool fdi_apollo2_call_helper_function(fdi_serial_wire_t *serial_wire, uint32_t address, uint32_t *parameters, uint32_t parameter_count, fdi_serial_wire_debug_error_t *error) {
    // store parameters at start of flash
    uint32_t parameter_address = 0x10000000;
    for (uint32_t i = 0; i < parameter_count; ++i) {
        if (!fdi_serial_wire_debug_write_memory_uint32(serial_wire, parameter_address, parameters[i], error)) {
            return false;
        }
        parameter_address += 4;
    }
    // initialize return code
    if (!fdi_serial_wire_debug_write_memory_uint32(serial_wire, parameter_address, 0xffffffff, error)) {
        return false;
    }

    // set stack pointer to top of RAM
    if (!fdi_serial_wire_debug_write_register(serial_wire, CORTEX_M_REGISTER_SP, 0x10040000, error)) {
        return false;
    }
    // set program counter to flash_mass_erase_sram
    if (!fdi_serial_wire_debug_write_register(serial_wire, CORTEX_M_REGISTER_PC, address, error)) {
        return false;
    }

    // run
    uint32_t dhcsr = SWD_DHCSR_DBGKEY | SWD_DHCSR_CTRL_DEBUGEN | SWD_DHCSR_CTRL_MASKINTS;
    if (!fdi_serial_wire_debug_write_memory_uint32(serial_wire, SWD_MEMORY_DHCSR, dhcsr, error)) {
        return false;
    }

    // wait for halt
    bool halted = false;
    do {
        if (!fdi_serial_wire_debug_is_halted(serial_wire, &halted, error)) {
            return false;
        }
    } while (!halted);

    // get and check return code
    uint32_t return_code = 0xffffffff;
    if (!fdi_serial_wire_debug_read_memory_uint32(serial_wire, parameter_address, &return_code, error)) {
        return false;
    }
    if (return_code != 0) {
        error->code = 1;
        error->detail = return_code;
        return false;
    }
    return true;
}

#define PROGRAM_KEY (0x12344321)

/****************************************//**
* @brief entry point for reading mass erase parameters from SRAM and then erasing the main block *
* Calling this function looks up programming information from offset 0x0 in SRAM
* 0x10000000 Instance number
* For flash instance addresses 0 – 0x7FFFF, this parameter should be 0.
* For flash instance addresses 0x80000 – 0xFFFFF, this parameter should be 1.
* 0x10000004 Program key to pass to flash helper routine
* 0x10000008 Return code, debugger sets this to -1, all RCs are >= 0
* Note this routine spins when flash_mass_erase() returns and waits for the debugger surrogate in the parallel programmer.
*
* @return never returns, spins here waiting for debugger or debugger surrogate on the parallel programmer
*
*******************************************/
bool fdi_apollo2_flash_mass_erase_from_sram(fdi_serial_wire_t *serial_wire, uint32_t instance, fdi_serial_wire_debug_error_t *error) {
    uint32_t parameters[] = {instance, PROGRAM_KEY};
    return fdi_apollo2_call_helper_function(serial_wire, 0x08000069, parameters, sizeof(parameters) / sizeof(parameters[0]), error);
}

/****************************************//**
* @brief entry point for reading program details and data from SRAM and then programming it into MAIN *
* Calling this function looks up programming parameters starting at offset 0x0 in SRAM
* 0x10000000 pointer in to flash
* 0x10000004 number of 32-bit words to program
* 0x10000008 PROGRAM key to pass to flash helper routine
* 0x1000000C return code debugger sets this to -1 all RCs are >= 0
* 0x10001000 first 32-bit word of data buffer to be programmed (WRITE_BUFFER_START)
* Note this routine hits a break point instruction to transfer control to a debugger surrogate in the parallel programmer. *
* @return never returns, spins here waiting for debugger or debugger surrogate on the parallel programmer *******************************************/
#define WRITE_BUFFER_START (0x10001000)
bool fdi_apollo2_flash_program_main_from_sram(fdi_serial_wire_t *serial_wire, uint32_t address, uint8_t *data, uint32_t length, fdi_serial_wire_debug_error_t *error) {
    if (!fdi_serial_wire_debug_write_data(serial_wire, WRITE_BUFFER_START, data, length, error)) {
        return false;
    }
    uint32_t parameters[] = {address, length / 4, PROGRAM_KEY};
    return fdi_apollo2_call_helper_function(serial_wire, 0x0800005d, parameters, sizeof(parameters) / sizeof(parameters[0]), error);
}

/****************************************//**
* @brief entry point for reading mass erase parameters from SRAM and then erasing an entire
* (customer) INFO instance. *
* Calling this function looks up programming information from offset 0x0 in SRAM
* 0x10000000 INFO instance number, 0 or 1.
* 0x10000004 Program key to pass to flash helper routine
Page 138 of 557
2017 Ambiq Micro, Inc. All rights reserved.
Apollo2 Datasheet
    
 * 0x10000008 Return code, debugger sets this to -1, all RCs are >= 0 *
* Note this routine spins when flash_info_erase() returns and waits for the debugger surrogate in the parallel programmer.
*
* @return never returns, spins here waiting for debugger or debugger surrogate on the parallel programmer *
*
*******************************************/
bool fdi_apollo2_flash_info_erase_from_sram(fdi_serial_wire_t *serial_wire, fdi_serial_wire_debug_error_t *error) {
    uint32_t parameters[] = {0, PROGRAM_KEY};
    return fdi_apollo2_call_helper_function(serial_wire, 0x08000085, parameters, sizeof(parameters) / sizeof(parameters[0]), error);
}

/****************************************//**
* @brief entry point for reading program details and data from SRAM and then programming it into the OTP *
* Calling this function looks up programming parameters starting at offset 0x0 in SRAM
* 0x10000000 Word offset in to the INFO space, 0 <= Offset < 256
* 0x10000004 Instance number, 0 or 1 (typically 0)
* 0x10000008 Number of 32-bit words to program
* 0x1000000C Program key to pass to flash helper routine
* 0x10000010 Return code. Debugger sets this to -1, all RCs are >= 0
* 0x10001000 first 32-bit word of data buffer to be programmed (WRITE_BUFFER_START) *
* Note this routine hits a break point instruction to transfer control to a debugger surrogate in the parallel programmer. *
* @return never returns, spins here waiting for debugger or debugger surrogate on the parallel programmer
*
*******************************************/
bool fdi_apollo2_flash_program_info_area_from_sram(fdi_serial_wire_t *serial_wire, uint32_t offset, uint8_t *data, uint32_t length, fdi_serial_wire_debug_error_t *error) {
    if (!fdi_serial_wire_debug_write_data(serial_wire, WRITE_BUFFER_START, data, length, error)) {
        return false;
    }
    uint32_t parameters[] = {offset, 0, length / 4, PROGRAM_KEY};
    return fdi_apollo2_call_helper_function(serial_wire, 0x08000061, parameters, sizeof(parameters) / sizeof(parameters[0]), error);
}

bool fdi_apollo2_test(fdi_serial_wire_t *serial_wire) {
    fdi_serial_wire_debug_error_t error;
    bool result;

    fdi_serial_wire_set_reset(serial_wire, true);
    fdi_delay_ms(100);

    fdi_serial_wire_debug_reset_debug_port(serial_wire);

    uint32_t dpid;
    result = fdi_serial_wire_debug_read_debug_port(serial_wire, SWD_DP_IDCODE, &dpid, &error);

    result = fdi_serial_wire_debug_initialize_debug_port(serial_wire, &error);

    result = fdi_serial_wire_debug_initialize_access_port(serial_wire, &error);

    result = fdi_serial_wire_debug_halt(serial_wire, &error);
    uint32_t dbg_hcsr = 0;
    result = fdi_serial_wire_debug_read_memory_uint32(serial_wire, SWD_MEMORY_DHCSR, &dbg_hcsr, &error);
    result = fdi_serial_wire_debug_read_memory_uint32(serial_wire, SWD_MEMORY_DHCSR, &dbg_hcsr, &error);

    uint32_t chip_info = 0xffffffff;
    result = fdi_serial_wire_debug_read_memory_uint32(serial_wire, 0x40020000 /* CHIP_INFO */, &chip_info, &error);
    uint32_t chip_rev = 0xffffffff;
    result = fdi_serial_wire_debug_read_memory_uint32(serial_wire, 0x4002000C /* CHIPREV */, &chip_rev, &error);
    uint32_t vendor_id = 0xffffffff;
    result = fdi_serial_wire_debug_read_memory_uint32(serial_wire, 0x40020010 /* VENDORID */, &vendor_id, &error);
    uint32_t debugger = 0xffffffff;
    result = fdi_serial_wire_debug_read_memory_uint32(serial_wire, 0x40020014 /* DEBUGGER */, &debugger, &error);

    uint32_t address = 0x10002000;
    uint32_t value = 0xffffffff;
    result = fdi_serial_wire_debug_read_memory_uint32(serial_wire, address, &value, &error);
    result = fdi_serial_wire_debug_write_memory_uint32(serial_wire, address, 0x12345678, &error);
    value = 0xffffffff;
    result = fdi_serial_wire_debug_read_memory_uint32(serial_wire, address, &value, &error);

    address = 0x10002004;
    uint8_t data[] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0};
    result = fdi_serial_wire_debug_write_data(serial_wire, address, data, sizeof(data), &error);
    uint8_t verify[sizeof(data)] = {0, };
    result = fdi_serial_wire_debug_read_data(serial_wire, address, verify, sizeof(verify), &error);

    uint16_t register_id = 0; // R0
    result = fdi_serial_wire_debug_write_register(serial_wire, register_id, 0x12345678, &error);
    value = 0xffffffff;
    result = fdi_serial_wire_debug_read_register(serial_wire, register_id, &value, &error);

    result = fdi_apollo2_flash_mass_erase_from_sram(serial_wire, 0 /* instance */, &error);
    address = 0x00002000;
    result = fdi_serial_wire_debug_read_data(serial_wire, address, verify, sizeof(verify), &error);
    result = fdi_apollo2_flash_program_main_from_sram(serial_wire, address, data, sizeof(data), &error);
    result = fdi_serial_wire_debug_read_data(serial_wire, address, verify, sizeof(verify), &error);

    return result;
}