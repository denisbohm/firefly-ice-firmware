#ifndef FDI_I2CS_H
#define FDI_I2CS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t address;
    void (*rx)(uint8_t *data, size_t size);
    void (*tx_ready)(void);
} fdi_i2cs_configuration_t;

void fdi_i2cs_initialize(fdi_i2cs_configuration_t configuration);

bool fdi_i2cs_can_transmit(void);
void fdi_i2cs_transmit(uint8_t *data, size_t size);

#endif