#ifndef FD_I2C1_H
#define FD_I2C1_H

#include <stdbool.h>
#include <stdint.h>

void fd_i2c1_initialize(void);

void fd_i2c1_power_on(void);

bool fd_i2c1_read(uint8_t address, uint8_t reg, uint8_t *presult);

#endif