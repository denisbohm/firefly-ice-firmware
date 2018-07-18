#include <stdint.h>

#include <am_mcu_apollo.h>
#include <am_util.h>

void halt(void) {
    __asm("BKPT   #0");
}

#define OPTION_ERASE_BEFORE_WRITE 0x00000001

void write_pages(uint32_t *address, uint32_t *data, uint32_t pages, uint32_t options) {
    uint32_t page_size = AM_HAL_FLASH_PAGE_SIZE;
    uint32_t words_per_page = page_size / sizeof(uint32_t);
    for (uint32_t i = 0; i < pages; ++i) {
        if (options & OPTION_ERASE_BEFORE_WRITE) {
            uint32_t addr = (uint32_t)address;
            uint32_t result = am_hal_flash_page_erase(AM_HAL_FLASH_PROGRAM_KEY, AM_HAL_FLASH_ADDR2INST(addr), AM_HAL_FLASH_ADDR2PAGE(addr));
        }
        uint32_t result = am_hal_flash_program_main(AM_HAL_FLASH_PROGRAM_KEY, (void *)data, address, page_size);
        address += words_per_page;
        data += words_per_page;
    }
}

void main(void) {
#if 0
    static uint8_t bytes[8192];

    for (int i = 0; i < sizeof(bytes); ++i) {
        bytes[i] = i;
    }
    write_pages((uint32_t *)0x10000, (uint32_t *)bytes, 1, OPTION_ERASE_BEFORE_WRITE);
#endif

    write_pages(0,0,0,0);
    halt();
}
