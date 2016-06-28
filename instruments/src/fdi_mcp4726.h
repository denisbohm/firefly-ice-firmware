#ifndef FDI_MCP4726_H
#define FDI_MCP4726_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t gain:1;
    uint8_t power_down:2;
    uint8_t voltage_reference:2;
    uint8_t zero:1;
    uint8_t power_on_reset:1;
    uint8_t ready:1;
} fdi_mcp4726_read_status_t;

typedef struct {
    union {
        fdi_mcp4726_read_status_t status;
        uint8_t status_byte;
    };
    uint16_t data;
} fdi_mcp4726_read_t;

bool fdi_mcp4726_write_volatile_dac_register(uint16_t data);

bool fdi_mcp4726_read(fdi_mcp4726_read_t *read);

#endif
