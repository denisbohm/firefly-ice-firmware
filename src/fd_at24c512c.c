#include "fd_at24c512c.h"
#include "fd_i2c1.h"
#include "fd_log.h"

#include <string.h>

#define ADDRESS 0xa0

void fd_at24c512c_initialize(void) {
}

void fd_at24c512c_erase_page(uint32_t address) {
    uint8_t data[FD_AT24C512C_PAGE_SIZE];
    memset(data, 0xff, FD_AT24C512C_PAGE_SIZE);
    fd_at24c512c_write_page(address, data, FD_AT24C512C_PAGE_SIZE);
}

void fd_at24c512c_write_page(uint32_t address, uint8_t *data, uint32_t length) {
    fd_i2c1_write_bytes(ADDRESS, address, data, length);
}

void fd_at24c512c_read_page(uint32_t address, uint8_t *data, uint32_t length) {
    fd_i2c1_read_bytes(ADDRESS, address, data, length);
}

void fd_at24c512c_test(void) {
    uint8_t write_data[2] = {0x01, 0x02};
    fd_i2c1_write_bytes(ADDRESS, 0x0000, write_data, sizeof(write_data));

    uint8_t read_data[2] = {0x00, 0x00};
    fd_i2c1_read_bytes(ADDRESS, 0x0000, read_data, sizeof(read_data));

    if (write_data[0] != read_data[0]) {
        fd_log_ram_assert_fail("");
        return;
    }
}