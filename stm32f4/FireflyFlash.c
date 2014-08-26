#include <stm32f4xx.h>

#include <stdbool.h>
#include <stdint.h>

void halt(void) {
    __asm("BKPT   #0");
}

#define OPTION_ERASE_BEFORE_WRITE 0x00000001

#define SECTOR_MASK               ((uint32_t)0xFFFFFF07)

#define FLASH_FLAG_BSY                 ((uint32_t)0x00010000)  /*!< FLASH Busy flag                           */ 

#define FLASH_PSIZE_WORD           ((uint32_t)0x00000200)
#define CR_PSIZE_MASK              ((uint32_t)0xFFFFFCFF)

#define FLASH_KEY1               ((uint32_t)0x45670123)
#define FLASH_KEY2               ((uint32_t)0xCDEF89AB)

#define FLASH_Sector_0     ((uint16_t)0x0000) /*!< Sector Number 0   */
#define FLASH_Sector_1     ((uint16_t)0x0008) /*!< Sector Number 1   */
#define FLASH_Sector_2     ((uint16_t)0x0010) /*!< Sector Number 2   */
#define FLASH_Sector_3     ((uint16_t)0x0018) /*!< Sector Number 3   */
#define FLASH_Sector_4     ((uint16_t)0x0020) /*!< Sector Number 4   */
#define FLASH_Sector_5     ((uint16_t)0x0028) /*!< Sector Number 5   */
#define FLASH_Sector_6     ((uint16_t)0x0030) /*!< Sector Number 6   */
#define FLASH_Sector_7     ((uint16_t)0x0038) /*!< Sector Number 7   */

static
bool is_sector_start(uint32_t address, uint32_t *sector) {
    switch (address) {
        case 0x08000000: // to 0x0800 3FFF 16 Kbytes
            *sector = FLASH_Sector_0;
            return true;
        case 0x08004000: // to 0x0800 7FFF 16 Kbytes
            *sector = FLASH_Sector_1;
            return true;
        case 0x08008000: // to 0x0800 BFFF 16 Kbytes
            *sector = FLASH_Sector_2;
            return true;
        case 0x0800C000: // to 0x0800 FFFF 16 Kbytes
            *sector = FLASH_Sector_3;
            return true;
        case 0x08010000: // to 0x0801 FFFF 64 Kbytes
            *sector = FLASH_Sector_4;
            return true;
        case 0x08020000: // to 0x0803 FFFF 128 Kbytes
            *sector = FLASH_Sector_5;
            return true;
        case 0x08040000: // to 0x0805 FFFF 128 Kbytes
            *sector = FLASH_Sector_6;
            return true;
        case 0x08060000: // to 0x0807 FFFF 128 Kbytes
            *sector = FLASH_Sector_7;
            return true;
    }
    return false;
}

void write_pages(uint32_t *address, uint32_t *data, uint32_t pages, uint32_t options) {
    uint32_t page_size = 0x800; // STM32F4 doesn't have a fixed page size, so use an arbitrary 2k page size -denis
    uint32_t words_per_page = page_size / sizeof(uint32_t);

    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }  

    uint32_t sector;
    if ((options & OPTION_ERASE_BEFORE_WRITE) && is_sector_start((uint32_t)address, &sector)) {
        while (FLASH->SR & FLASH_FLAG_BSY);

        FLASH->CR &= CR_PSIZE_MASK;
        FLASH->CR |= FLASH_PSIZE_WORD;
        FLASH->CR &= SECTOR_MASK;
        FLASH->CR |= FLASH_CR_SER | sector;
        FLASH->CR |= FLASH_CR_STRT;

        while (FLASH->SR & FLASH_FLAG_BSY);

        FLASH->CR &= (~FLASH_CR_SER);
        FLASH->CR &= SECTOR_MASK; 
    }

    for (uint32_t i = 0; i < pages; ++i) {
        for (uint32_t j = 0; j < words_per_page; ++j) {
            while (FLASH->SR & FLASH_FLAG_BSY);

            FLASH->CR &= CR_PSIZE_MASK;
            FLASH->CR |= FLASH_PSIZE_WORD;
            FLASH->CR |= FLASH_CR_PG;
  
            *address = *data;
        
            while (FLASH->SR & FLASH_FLAG_BSY);

            FLASH->CR &= (~FLASH_CR_PG);

            ++address;
            ++data;
        }
    }

    FLASH->CR |= FLASH_CR_LOCK;
}

void main(void) {
#if 0
    static uint8_t bytes[0x800];

    for (int i = 0; i < sizeof(bytes); ++i) {
        bytes[i] = i;
    }
    write_pages((uint32_t *)0x08004000, (uint32_t *)bytes, 1, OPTION_ERASE_BEFORE_WRITE);
#endif

    write_pages(0,0,0,0);
    halt();
}
