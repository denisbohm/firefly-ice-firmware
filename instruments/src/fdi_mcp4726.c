#include "fdi_mcp4726.h"

#include "fdi_i2c.h"

const uint8_t fdi_mcp4726a1_address = 0b11000010;

bool fdi_mcp4726_write_volatile_dac_register(uint16_t data) {
    const uint8_t command = 0b00000000;
    const uint8_t power_down = 0b00000000;
    uint8_t bytes[] = {command | power_down | ((data >> 8) & 0b00001111), data};
    return fdi_i2c_write(fdi_mcp4726a1_address, bytes, sizeof(bytes));
}

bool fdi_mcp4726_read(fdi_mcp4726_read_t *read) {
    uint8_t bytes[6];
    bool result = fdi_i2c_read(fdi_mcp4726a1_address, bytes, sizeof(bytes));
    read->status_byte = bytes[0];
    read->data = (bytes[1] << 4) | (bytes[2] >> 4);
    return result;
}