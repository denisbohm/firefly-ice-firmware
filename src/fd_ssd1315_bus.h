#ifndef fd_ssd1315_bus_h
#define fd_ssd1315_bus_h

#include <stdint.h>

void fd_ssd1315_bus_initialize(void);
void fd_ssd1315_bus_read(uint8_t *data, uint32_t length);
void fd_ssd1315_bus_write(const uint8_t *data, uint32_t length);
void fd_ssd1315_bus_write_pixel(const uint8_t *data, uint32_t length);

#endif
