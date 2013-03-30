#ifndef FD_I2C1_H
#define FD_I2C1_H

#include <stdbool.h>
#include <stdint.h>

void fd_i2c1_initialize(void);

void fd_i2c1_power_on(void);

bool fd_i2c1_read(uint8_t address, uint8_t reg, uint8_t *presult);

bool fd_i2c1_read_bytes(uint8_t device, uint16_t address, uint8_t *buffer, uint32_t length);
bool fd_i2c1_write_bytes(uint8_t device, uint16_t address, uint8_t *buffer, uint32_t length);
bool fd_i2c1_poll(uint8_t device);

#endif