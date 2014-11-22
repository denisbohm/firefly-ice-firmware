#include <stdint.h>

#include "nano1xx_fmc.h"

void halt(void) {
//    *(uint32_t *)0xE000EDF0 = 0xA05F0003; // DHCSR = DBGKEY | C_HALT | C_DEBUGEN;
    __asm(  ".syntax unified\n"
            "BKPT   #0\n"
            ".syntax divided\n");
}

static int32_t UnlockProtectedReg(void)
{
        uint32_t lockbit;

        GCR->RegLockAddr = 0x59;
        GCR->RegLockAddr = 0x16;
        GCR->RegLockAddr = 0x88;

        lockbit = GCR->RegLockAddr & 0x01;
        return (lockbit);
}

#define OPTION_ERASE_BEFORE_WRITE 0x00000001

#define WORDS_PER_PAGE (PAGE_SIZE / sizeof(uint32_t))

void write_pages(uint32_t *address, uint32_t *data, uint32_t pages, uint32_t options) {
    UnlockProtectedReg();
    FMC_EnableAPUpdate();
    FMC_Init();
    for (uint32_t i = 0; i < pages; ++i) {
        if (options & OPTION_ERASE_BEFORE_WRITE) {
            FMC_Erase((uint32_t)address);
        }
        for (uint32_t j = 0; j < WORDS_PER_PAGE; ++j) {
            uint32_t word = *data;
            FMC_Write((uint32_t)address, word);
            ++address;
            ++data;
        }
    }
    FMC_DeInit();
}

void main(void) {
/*
    static uint8_t bytes[512];
    for (int i = 0; i < sizeof(bytes); ++i) {
        bytes[i] = ~i;
    }
    uint32_t address = 0x0000;
    for (int i = 0; i < 24; ++i) {
        write_pages((uint32_t *)address, (uint32_t *)bytes, 1, OPTION_ERASE_BEFORE_WRITE);
        address += 512;
    }
    uint32_t out = *((uint32_t *)address);
*/
    halt();
    write_pages(0,0,0,0);
}