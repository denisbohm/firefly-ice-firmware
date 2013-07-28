#ifndef FD_W25Q16DW_H
#define FD_W25Q16DW_H

#include <stdint.h>

#define FD_W25Q16DW_PAGE_SIZE 256
//#define FD_W25Q16DW_PAGES 8192
// !!! just for testing using a RAM simulation -denis
#define FD_W25Q16DW_PAGES 32
#define FD_W25Q16DW_PAGES_PER_SECTOR 16

void fd_w25q16dw_initialize(void);

void fd_w25q16dw_sleep(void);
void fd_w25q16dw_wake(void);

void fd_w25q16dw_enable_write(void);

// erase a 4K-byte sector
void fd_w25q16dw_erase_sector(uint32_t address);

// write up to 256-bytes within a page
void fd_w25q16dw_write_page(uint32_t address, uint8_t *data, uint32_t length);

void fd_w25q16dw_read(uint32_t address, uint8_t *data, uint32_t length);

#endif