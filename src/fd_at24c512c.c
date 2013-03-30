#include "fd_at24c512c.h"
#include "fd_i2c1.h"

#define ADDRESS 0xa0

void fd_at24c512c_initialize(void) {
}

void fd_at24c512c_test(void) {
    uint8_t write_data[2] = {0x01, 0x02};
    fd_i2c1_write_bytes(ADDRESS, 0x0000, write_data, sizeof(write_data));
    fd_i2c1_poll(ADDRESS);

    uint8_t read_data[2] = {0x00, 0x00};
    fd_i2c1_read_bytes(ADDRESS, 0x0000, read_data, sizeof(read_data));

    if (write_data[0] != read_data[0]) {
        // log diagnostic
        return;
    }
}