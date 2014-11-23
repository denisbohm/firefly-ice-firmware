#include "fd_hal_external_flash.h"

#include "fd_w25q16dw.h"

uint32_t fd_hal_external_flash_get_pages(void) {
    return FD_W25Q16DW_PAGES;
}

uint32_t fd_hal_external_flash_get_pages_per_sector(void) {
    return FD_W25Q16DW_PAGES_PER_SECTOR;
}

void fd_hal_external_flash_sleep(void) {
    fd_w25q16dw_sleep();
}

void fd_hal_external_flash_wake(void) {
    fd_w25q16dw_wake();
}

void fd_hal_external_flash_enable_write(void) {
    fd_w25q16dw_enable_write();
}

void fd_hal_external_flash_erase_sector(uint32_t address) {
    fd_w25q16dw_erase_sector(address);
}

void fd_hal_external_flash_write_page(uint32_t address, uint8_t *data, uint32_t length) {
    fd_w25q16dw_write_page(address, data, length);
}

void fd_hal_external_flash_read(uint32_t address, uint8_t *data, uint32_t length) {
    fd_w25q16dw_read(address, data, length);
}

void fd_hal_external_flash_wait_while_busy(void) {
    fd_w25q16dw_wait_while_busy();
}
