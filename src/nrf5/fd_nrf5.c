#include "fd_nrf5.h"

uint32_t fd_nrf5_error_count = 0;

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t *p_file_name) {
    ++fd_nrf5_error_count;
}

void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name) {
    ++fd_nrf5_error_count;
}
