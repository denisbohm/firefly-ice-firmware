#include "nrf51.h"
#include "nrf51_bitfields.h"

#include <stdbool.h>
#include <stdint.h>

void halt(void) {
    __asm("BKPT   #0");
}

#define OPTION_ERASE_BEFORE_WRITE 0x00000001

#define is_page_start(a) ((a & 0x3ff) == 0)

void nrf_nvmc_page_erase(uint32_t address) { 
    // Enable erase.
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
    }

    // Erase the page
    NRF_NVMC->ERASEPAGE = address;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
    }

    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
    }
}

void nrf_nvmc_write_words(uint32_t address, const uint32_t *src, uint32_t num_words) {
    // Enable write.
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
    }

    for (uint32_t i = 0; i < num_words; i++) {
        ((uint32_t*)address)[i] = src[i];
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
        }
    }

    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
    }
}

void write_pages(uint32_t address, uint32_t *data, uint32_t pages, uint32_t options) {
    uint32_t page_size = 0x400; // nRF51 has a fixed page size of 1KB -denis
    uint32_t words_per_page = page_size / sizeof(uint32_t);

    for (uint32_t i = 0; i < pages; ++i) {
        if ((options & OPTION_ERASE_BEFORE_WRITE) && is_page_start(address)) {
            nrf_nvmc_page_erase(address);
        }

        nrf_nvmc_write_words(address, data, words_per_page);
        address += page_size;
        data += words_per_page;
    }
}

void main(void) {
#if 0
    static uint8_t bytes[0x400];

    for (int i = 0; i < sizeof(bytes); ++i) {
        bytes[i] = i;
    }
    write_pages(0x37C00, (uint32_t *)bytes, 1, OPTION_ERASE_BEFORE_WRITE);
#endif

    write_pages(0,0,0,0);
    halt();
}
