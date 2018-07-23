#include "fdi_serial_wire_flash.h"

#include "fdi_delay.h"
#include "fdi_firefly_flash.h"
#include "fdi_firefly_flash_apollo.h"
#include "fdi_gpio.h"
#include "fdi_serial_wire_debug.h"

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

bool fdi_serial_wire_is_halted(fdi_serial_wire_t *serial_wire, bool *halted, fdi_serial_wire_debug_error_t *error) {
    uint32_t dhcsr;
    if (!fdi_serial_wire_debug_read_memory_uint32(serial_wire, SWD_MEMORY_DHCSR, &dhcsr, error)) {
        return false;
    }
    *halted = (dhcsr & SWD_DHCSR_STAT_HALT) != 0;
    return true;
}

typedef struct {
    uint32_t register_id;
    uint32_t value;
} fdi_serial_wire_flash_register_value_t;

#define OPTION_ERASE_BEFORE_WRITE 0x00000001

bool fdi_serial_wire_flash_write_pages(
    fdi_serial_wire_flash_t *flash,
    uint32_t address,
    uint32_t length,
    fdi_serial_wire_flash_reader_t reader,
    void *reader_context,
    uint32_t reader_address,
    fdi_serial_wire_debug_error_t *error
) {
    // halt the target MCU
    uint32_t dhcsr = SWD_DHCSR_DBGKEY | SWD_DHCSR_CTRL_DEBUGEN | SWD_DHCSR_CTRL_HALT;
    if (!fdi_serial_wire_debug_write_memory_uint32(flash->serial_wire, SWD_MEMORY_DHCSR, dhcsr, error)) {
        return false;
    }

    // transfer data to be written to flash into a RAM buffer
    const uint32_t buffer_address = flash->rpc.heap.address;
    uint32_t src_address = reader_address;
    uint32_t dst_address = buffer_address;
    uint32_t remaining = length;
    uint8_t data[256];
    while (remaining > 0) {
        uint32_t data_length = remaining;
        if (data_length > sizeof(data)) {
            data_length = sizeof(data);
        }
        reader(reader_context, src_address, data_length, data);
        if (!fdi_serial_wire_debug_write_data(flash->serial_wire, dst_address, data, data_length, error)) {
            return false;
        }
        src_address += data_length;
        dst_address += data_length;
        remaining -= data_length;
    }

    fdi_serial_wire_flash_register_value_t writes[] = {
        { .register_id = CORTEX_M_REGISTER_R0, .value = address }, // address
        { .register_id = CORTEX_M_REGISTER_R1, .value = buffer_address }, // data
        { .register_id = CORTEX_M_REGISTER_R2, .value = length / flash->firefly_flash->page_length }, // page count
        { .register_id = CORTEX_M_REGISTER_R3, .value = OPTION_ERASE_BEFORE_WRITE }, // options
        { .register_id = CORTEX_M_REGISTER_SP, .value = flash->rpc.stack.address + flash->rpc.stack.length },
        { .register_id = CORTEX_M_REGISTER_PC, .value = flash->firefly_flash->write_pages_address },
        { .register_id = CORTEX_M_REGISTER_LR, .value = flash->rpc.halt_address | 0x00000001 },
    };
    for (int i = 0; i < sizeof(writes) / sizeof(writes[0]); ++i) {
        fdi_serial_wire_flash_register_value_t write = writes[i];
        if (!fdi_serial_wire_debug_write_register(flash->serial_wire, write.register_id, write.value, error)) {
            return false;
        }
    }

    // run the target MCU
    dhcsr = SWD_DHCSR_DBGKEY | SWD_DHCSR_CTRL_DEBUGEN | SWD_DHCSR_CTRL_MASKINTS;
    if (!fdi_serial_wire_debug_write_memory_uint32(flash->serial_wire, SWD_MEMORY_DHCSR, dhcsr, error)) {
        return false;
    }

    // wait for target MCU to halt
    bool halted = false;
    while (!halted) {
        if (!fdi_serial_wire_is_halted(flash->serial_wire, &halted, error)) {
            return false;
        }
    }

    return true;
}

bool fdi_serial_wire_flash(fdi_serial_wire_flash_t *flash, fdi_serial_wire_debug_error_t *error) {
    // hard reset target MCU
    fdi_gpio_set(FDI_GPIO_ATE_SWD1_NRESET, false);
    fdi_delay_ms(100);
    fdi_gpio_set(FDI_GPIO_ATE_SWD1_NRESET, true);
    fdi_delay_ms(100);

    // bring up the SWD port
    fdi_serial_wire_t *serial_wire = flash->serial_wire;
    fdi_serial_wire_debug_reset_debug_port(serial_wire);
    uint32_t dpid;
    if (!fdi_serial_wire_debug_read_debug_port(serial_wire, SWD_DP_IDCODE, &dpid, error)) {
        return false;
    }
    if (!fdi_serial_wire_debug_initialize_debug_port(serial_wire, error)) {
        return false;
    }

#if 0
    uint32_t address = 0x20000000;
    uint32_t value = 0;
    result = fdi_serial_wire_debug_read_memory_uint32(serial_wire, address, &value, &error);
    result = fdi_serial_wire_debug_write_memory_uint32(serial_wire, address, 0x12345678, &error);
    result = fdi_serial_wire_debug_read_memory_uint32(serial_wire, address, &value, &error);

    uint8_t data[] = {0x12, 0x34, 0x56, 0x78};
    result = fdi_serial_wire_debug_write_data(serial_wire, address, data, sizeof(data), &error);
    uint8_t verify[] = {0, 0, 0, 0};
    bool result = fdi_serial_wire_debug_read_data(serial_wire, address, verify, sizeof(verify), &error);
#endif

    // load FireflyFlash<MCU> into target RAM
    fdi_firefly_flash_t *firefly_flash = flash->firefly_flash;
    if (!fdi_serial_wire_debug_write_data(serial_wire, firefly_flash->executable_range.address, firefly_flash->executable_data, firefly_flash->executable_range.length, error)) {
        return false;
    }

    // RPC calls to FireflyFlash to flash pages one at a time
    uint32_t src_address = flash->reader_address;
    uint32_t dst_address = flash->address;
    uint32_t remaining = flash->length;
    while (remaining > 0) {
        uint32_t data_length = remaining;
        if (data_length > firefly_flash->page_length) {
            data_length = firefly_flash->page_length;
        }
        if (!fdi_serial_wire_flash_write_pages(flash, dst_address, data_length, flash->reader, 0, src_address, error)) {
            return false;
        }
        src_address += data_length;
        dst_address += data_length;
        remaining -= data_length;
    }

    return true;
}

bool fdi_serial_wire_flash_test(void) {
    uint32_t ram_address = 0x10000000;
    uint32_t ram_length = 0x40000;
    fdi_firefly_flash_t *firefly_flash = &fdi_firefly_flash_apollo;
    uint32_t free = firefly_flash->executable_range.address + firefly_flash->executable_range.length;
    free = 16 * ((free + 15) / 16); // stack should be on 16 byte boundary
    uint32_t stack = free;
    uint32_t stack_length = 256;
    free += stack_length;
    uint32_t heap = free;
    uint32_t heap_length = ram_address + ram_length - heap;
    fdi_serial_wire_flash_t flash = {
        .serial_wire = &fdi_serial_wires[0],
        .firefly_flash = firefly_flash,
        .rpc = {
            .ram = { .address = ram_address, .length = ram_length },
            .stack = { .address = stack, .length = stack_length },
            .heap = { .address = heap, .length = heap_length },
            .halt_address = firefly_flash->halt_address,
        },
        .address = 0x00000000,
        .length = 0,
        .reader = 0,
        .reader_address = 0,
    };

    fdi_serial_wire_debug_error_t error;
    bool result = fdi_serial_wire_flash(&flash, &error);
    return result;
}
