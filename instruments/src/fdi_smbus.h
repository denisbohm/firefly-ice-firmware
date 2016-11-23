#ifndef FDI_SMBUS_H
#define FDI_SMBUS_H

#include <stdbool.h>
#include <stdint.h>

void fdi_smbus_initialize(void);

bool fdi_smbus_write(uint8_t device_address, uint8_t register_address, uint8_t *data, uint32_t length);
bool fdi_smbus_read(uint8_t device_address, uint8_t register_address, uint8_t *data, uint32_t length);

#endif