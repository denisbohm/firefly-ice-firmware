#ifndef FD_HAL_EXTERNAL_FLASH_H
#define FD_HAL_EXTERNAL_FLASH_H

#include "fd_range.h"

#include <stdint.h>

#define FD_HAL_EXTERNAL_FLASH_PAGE_SIZE 256

uint32_t fd_hal_external_flash_get_pages(void);
uint32_t fd_hal_external_flash_get_pages_per_sector(void);

void fd_hal_external_flash_initialize(void);
void fd_hal_external_flash_sleep(void);
void fd_hal_external_flash_wake(void);
void fd_hal_external_flash_enable_write(void);
void fd_hal_external_flash_erase_sector(uint32_t address);
void fd_hal_external_flash_write_page(uint32_t address, uint8_t *data, uint32_t length);
void fd_hal_external_flash_read(uint32_t address, uint8_t *data, uint32_t length);
void fd_hal_external_flash_wait_while_busy(void);

fd_range_t fd_hal_external_flash_get_firmware_update_range(void);

#endif