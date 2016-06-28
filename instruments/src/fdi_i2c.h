#ifndef FDI_I2C_H
#define FDI_I2C_H

#include <stdbool.h>
#include <stdint.h>

void fdi_i2c_initialize(void);

bool fdi_i2c_write(uint8_t address, uint8_t* data, int length);
bool fdi_i2c_read(uint8_t address, uint8_t* data, int length);

#endif
