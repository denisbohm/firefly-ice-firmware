#include <stdint.h>

#include "em_device.h"

void halt(void) {
    *(uint32_t *)0xE000EDF0 = 0xA05F0003; // DHCSR = DBGKEY | C_HALT | C_DEBUGEN;
}

#define OPTION_ERASE_BEFORE_WRITE 0x00000001

#define MEM_INFO_PAGE_SIZE 0x0FE081E7

void write_pages(uint32_t *address, uint32_t *data, uint32_t pages, uint32_t options) {
    uint8_t mem_info_page_size = *(uint8_t *)MEM_INFO_PAGE_SIZE;
    uint32_t page_size = 1 << ((mem_info_page_size + 10) & 0xff);
    uint32_t words_per_page = page_size / sizeof(uint32_t);
    MSC->LOCK = MSC_UNLOCK_CODE;
    MSC->WRITECTRL |= MSC_WRITECTRL_WREN;
    for (uint32_t i = 0; i < pages; ++i) {
        if (options & OPTION_ERASE_BEFORE_WRITE) {
            MSC->ADDRB = (uint32_t) address;
            MSC->WRITECMD = MSC_WRITECMD_LADDRIM;
            MSC->WRITECMD = MSC_WRITECMD_ERASEPAGE;
            while (MSC->STATUS & MSC_STATUS_BUSY);
        }
        for (uint32_t j = 0; j < words_per_page; ++j) {
            MSC->ADDRB = (uint32_t)address;
            MSC->WRITECMD = MSC_WRITECMD_LADDRIM;
            while ((MSC->STATUS & MSC_STATUS_WDATAREADY) == 0);
            MSC->WDATA = *data;
            MSC->WRITECMD = MSC_WRITECMD_WRITETRIG;
            ++address;
            ++data;
            while (MSC->STATUS & MSC_STATUS_BUSY);
        }
    }
    MSC->WRITECTRL &= ~MSC_WRITECTRL_WREN;
    MSC->LOCK = 0;
}

void main(void) {
/*
    static uint8_t bytes[512];

    for (int i = 0; i < sizeof(bytes); ++i) {
        bytes[i] = i;
    }
    write_pages((uint32_t *)0x800, (uint32_t *)bytes, 1);
*/

    halt();
    write_pages(0,0,0,0);
}