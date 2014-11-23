#include "fd_boot.h"
#include "fd_hal_external_flash.h"
#include "fd_hal_processor.h"
#include "fd_hal_system.h"
#include "fd_log.h"
#include "fd_pins.h"
#include "fd_sha.h"
#include "fd_update.h"
#include "fd_w25q16dw.h"

#include <em_gpio.h>
#include <em_msc.h>

#include <stdbool.h>
#include <string.h>

#define INTERNAL_PAGE_SIZE 2048

static const fd_boot_data_t boot_data __attribute__ ((used, section(".boot_data"))) = {
    .magic = FD_BOOT_MAGIC,
    .major = 1,
    .minor = 0,
    .patch = 1,
    .capabilities = 0,
    .git_commit = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19}
};

bool is_metadata_valid(void) {
    uint32_t *address = (uint32_t *)fd_hal_processor_get_firmware_update_metadata_range(FD_UPDATE_AREA_APPLICATION).address;
    return *address != 0xffffffff;
}

void internal_read(uint32_t address, uint8_t *data, uint32_t length) {
    uint32_t *d = (uint32_t *)data;
    uint32_t *s = (uint32_t *)address;
    uint32_t *e = (uint32_t *)(address + length);
    while (s < e) {
        *d++ = *s++;
    }
}

bool is_internal_firmware_valid(void) {
    fd_update_metadata_t metadata;
    fd_update_read_metadata(FD_UPDATE_AREA_APPLICATION, &metadata);

    uint32_t firmware_address = fd_hal_processor_get_firmware_range(FD_UPDATE_AREA_APPLICATION).address;
    uint8_t internal_hash[FD_SHA_HASH_SIZE];
    fd_sha1(internal_read, firmware_address, metadata.length, internal_hash);

    return fd_sha1_is_equal(metadata.crypt_hash, internal_hash);
}

bool is_external_firmware_valid(void) {
    fd_update_metadata_t metadata;
    fd_update_read_metadata(FD_UPDATE_AREA_APPLICATION, &metadata);

    uint32_t data_base_address = fd_hal_system_get_firmware_update_range(FD_UPDATE_AREA_APPLICATION).address;
    uint8_t external_hash[FD_SHA_HASH_SIZE];
    fd_sha1(fd_w25q16dw_read, data_base_address, metadata.length, external_hash);

    return fd_sha1_is_equal(metadata.hash, external_hash);
}

void copy_firmware(void) {
    MSC_Init();

    fd_update_metadata_t metadata;
    fd_update_read_metadata(FD_UPDATE_AREA_APPLICATION, &metadata);

    uint32_t external_address = fd_hal_system_get_firmware_update_range(FD_UPDATE_AREA_APPLICATION).address;
    uint32_t internal_address = fd_hal_processor_get_firmware_range(FD_UPDATE_AREA_APPLICATION).address;
    uint32_t end = internal_address + metadata.length;
    uint8_t data[INTERNAL_PAGE_SIZE];
    while (internal_address < end) {
        fd_w25q16dw_read(external_address, data, INTERNAL_PAGE_SIZE);
        MSC_ErasePage((uint32_t *)internal_address);
        MSC_WriteWord((uint32_t *)internal_address, data, INTERNAL_PAGE_SIZE);

        external_address += INTERNAL_PAGE_SIZE;
        internal_address += INTERNAL_PAGE_SIZE;
    }

    MSC_Deinit();
}

void run_firmware(void) {
    uint32_t firmware_address = fd_hal_processor_get_firmware_range(FD_UPDATE_AREA_APPLICATION).address;
    SCB->VTOR = firmware_address;
    uint32_t *vector_table = (uint32_t *)firmware_address;
    uint32_t sp = vector_table[0];
    uint32_t pc = vector_table[1];
    __asm("msr msp, %0" : : "r" (sp));
    __asm("msr psp, %0" : : "r" (sp));
    __asm("mov pc, %0" : : "r" (pc));
}

int main(void) {
    fd_hal_processor_initialize();

    GPIO_PinOutClear(LED0_PORT_PIN);
    GPIO_PinOutClear(LED4_PORT_PIN);
    GPIO_PinOutClear(LED5_PORT_PIN);
    GPIO_PinOutClear(LED6_PORT_PIN);

    // workaround so that boot_data is not stripped (static used should work though according to gcc docs)... -denis
    uint8_t data[1];
    memcpy(data, boot_data.git_commit, sizeof(data));

    fd_log_initialize();
    fd_w25q16dw_initialize();
    fd_w25q16dw_wake();

    if (is_metadata_valid()) {
        if (is_internal_firmware_valid()) {
            run_firmware();
        } else
        if (is_external_firmware_valid()) {
            GPIO_PinOutSet(LED0_PORT_PIN);
            GPIO_PinOutSet(LED4_PORT_PIN);
            copy_firmware();
            if (is_internal_firmware_valid()) {
                GPIO_PinOutSet(LED5_PORT_PIN);
                run_firmware();
            }
        }
    }

    while (true) {
        GPIO_PinOutSet(LED0_PORT_PIN);
        fd_hal_processor_delay_ms(500);
        GPIO_PinOutClear(LED0_PORT_PIN);
        fd_hal_processor_delay_ms(500);
    }

    return 0;
}