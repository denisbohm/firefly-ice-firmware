#include "fd_hal_processor.h"

#include "fd_delay.h"
#include "fd_rtos.h"

#include <stddef.h>

uint32_t fd_hal_processor_interrupt_state;

void fd_hal_processor_interrupts_disable(void) {
    fd_hal_processor_interrupt_state = fd_rtos_interrupt_disable();
}

void fd_hal_processor_interrupts_enable(void) {
    fd_rtos_interrupt_enable(fd_hal_processor_interrupt_state);
}

void fd_hal_processor_sleep(void) {
}

void fd_hal_processor_wake(void) {
}

void fd_hal_processor_wait(void) {
}

void fd_hal_processor_initialize(void) {
}

void fd_hal_processor_delay_us(uint32_t us) {
    fd_delay_us(us);
}

void fd_hal_processor_delay_ms(uint32_t ms) {
    fd_delay_ms(ms);
}

bool fd_hal_processor_get_debug_lock(void) {
    return false;
}

void fd_hal_processor_set_debug_lock(void) {
}

uint32_t fd_hal_processor_get_model_number(void) {
    return 0;
}

void fd_hal_processor_get_hardware_id(fd_binary_t *binary) {
}

void fd_hal_processor_get_hardware_unique(fd_binary_t *binary) {
}

uint8_t *fd_hal_processor_get_provision_map_address(void) {
    return 0;
}

void fd_hal_processor_write_user_data(uint8_t *data, uint32_t length) {
}

void fd_hal_processor_write_flash_data(void *address, uint8_t *data, size_t length) {
}

void fd_hal_processor_erase_flash_data(void *address, size_t length __attribute__((unused))) {
}