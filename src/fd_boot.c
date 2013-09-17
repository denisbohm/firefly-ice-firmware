#include "fd_log.h"
#include "fd_processor.h"
#include "fd_sha.h"
#include "fd_update.h"
#include "fd_w25q16dw.h"

#include <em_gpio.h>
#include <em_msc.h>

#include <stdbool.h>

#define INTERNAL_PAGE_SIZE 2048

bool is_metadata_valid(void) {
    uint32_t *address = (uint32_t *)FD_UPDATE_METADATA_ADDRESS;
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
    fd_update_read_metadata(&metadata);

    uint8_t internal_hash[FD_SHA_HASH_SIZE];
    fd_sha1(internal_read, FD_UPDATE_FIRMWARE_ADDRESS, metadata.length, internal_hash);

    return fd_sha1_is_equal(metadata.crypt_hash, internal_hash);
}

bool is_external_firmware_valid(void) {
    fd_update_metadata_t metadata;
    fd_update_read_metadata(&metadata);

    uint8_t external_hash[FD_SHA_HASH_SIZE];
    fd_sha1(fd_w25q16dw_read, FD_UPDATE_DATA_BASE_ADDRESS, metadata.length, external_hash);

    return fd_sha1_is_equal(metadata.hash, external_hash);
}

void copy_firmware(void) {
    MSC_Init();

    fd_update_metadata_t metadata;
    fd_update_read_metadata(&metadata);

    uint32_t external_address = FD_UPDATE_DATA_BASE_ADDRESS;
    uint32_t internal_address = FD_UPDATE_FIRMWARE_ADDRESS;
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
    SCB->VTOR = FD_UPDATE_FIRMWARE_ADDRESS;
    uint32_t *vector_table = (uint32_t *)FD_UPDATE_FIRMWARE_ADDRESS;
    uint32_t sp = vector_table[0];
    uint32_t pc = vector_table[1];
    __asm("msr msp, %0" : : "r" (sp));
    __asm("msr psp, %0" : : "r" (sp));
    __asm("mov pc, %0" : : "r" (pc));
}

int main(void) {
    fd_processor_initialize();

    GPIO_PinOutClear(LED0_PORT_PIN);
    GPIO_PinOutClear(LED4_PORT_PIN);
    GPIO_PinOutClear(LED5_PORT_PIN);
    GPIO_PinOutClear(LED6_PORT_PIN);

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
//            if (is_internal_firmware_valid()) {
                GPIO_PinOutSet(LED5_PORT_PIN);
                run_firmware();
//            }
        }
    }

    while (true) {
        GPIO_PinOutSet(LED0_PORT_PIN);
        fd_delay_ms(500);
        GPIO_PinOutClear(LED0_PORT_PIN);
        fd_delay_ms(500);
    }

    return 0;
}