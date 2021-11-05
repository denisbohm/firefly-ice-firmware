#include "fd_ssd1315_bus.h"

#include "fd_i2cm.h"
#include "fd_log.h"

#define fd_ssd1315_i2c_address 0x78

void fd_ssd1315_bus_write(const uint8_t *data, uint32_t length) {
    
    const uint8_t header = 0x00;
    const uint8_t *end = data + length;
    while (data < end) {
        // ... to do -denis
    }
}

void fd_ssd1315_bus_read(uint8_t *data, uint32_t length) {
    fd_log_assert_fail("unsupported");
}

void fd_ssd1315_bus_write_pixel(const uint8_t *data, uint32_t length) {
    const uint8_t *end = data + length;
    while (data < end) {
        // ... to do -denis
    }
}

void fd_ssd1315_bus_initialize(void) {
}
