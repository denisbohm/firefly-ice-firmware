#ifndef FD_HAL_PROCESSOR_H
#define FD_HAL_PROCESSOR_H

#include "fd_binary.h"
#include "fd_boot.h"
#include "fd_map.h"
#include "fd_range.h"

#include <stdbool.h>
#include <stdint.h>

void fd_hal_processor_initialize(void);

void fd_hal_processor_sleep(void);
void fd_hal_processor_wake(void);

void fd_hal_processor_wait(void);

void fd_hal_processor_delay_ms(uint32_t ms);
void fd_hal_processor_delay_us(uint32_t ms);

void fd_hal_processor_interrupts_disable(void);
void fd_hal_processor_interrupts_enable(void);

bool fd_hal_processor_get_debug_lock(void);
void fd_hal_processor_set_debug_lock(void);

void fd_hal_processor_get_hardware_id(fd_binary_t *binary);

fd_range_t fd_hal_processor_get_boot_range(void);
fd_range_t fd_hal_processor_get_crypto_range(uint8_t area);
fd_range_t fd_hal_processor_get_firmware_update_metadata_range(uint8_t area);
fd_range_t fd_hal_processor_get_firmware_range(uint8_t area);

fd_boot_data_t *fd_hal_processor_get_boot_data_address(void);

uint8_t *fd_hal_processor_get_provision_map_address(void);

void fd_hal_processor_write_user_data(uint8_t *data, uint32_t length);

void fd_hal_processor_write_flash_data(uint32_t address, uint8_t *data, uint32_t length);

#endif
