#ifndef FD_HAL_PROCESSOR_H
#define FD_HAL_PROCESSOR_H

#include "fd_binary.h"
#include "fd_boot.h"
#include "fd_map.h"
#include "fd_range.h"

#include <stdbool.h>
#include <stddef.h>
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

void fd_hal_processor_get_hardware_id(fd_binary_t *binary); // deprecated
void fd_hal_processor_get_hardware_unique(fd_binary_t *binary);
uint32_t fd_hal_processor_get_model_number(void);
void fd_hal_processor_get_serial_number_string(uint8_t *serial_number, uint32_t length);
uint32_t fd_hal_processor_get_production_date(void);

uint8_t *fd_hal_processor_get_provision_map_address(void);

void fd_hal_processor_write_user_data(uint8_t *data, uint32_t length);

void fd_hal_processor_write_flash_data(void *address, uint8_t *data, size_t length);
void fd_hal_processor_erase_flash_data(void *address, size_t length);

#endif
