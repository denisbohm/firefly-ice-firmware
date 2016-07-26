#include "fdi_tcs3471.h"

#include "fdi_i2c.h"

const uint8_t fdi_tcs34711_address = 0x39 << 1;
const uint8_t fdi_tcs34715_address = 0x29 << 1;

const uint8_t fdi_tcs3471_command = 0b10000000;
const uint8_t fdi_tcs3471_command_type_auto_increment = 0b00100000;

const uint8_t fdi_tcs3471_register_enable = 0x00;
const uint8_t fdi_tcs3471_register_enable_wen = 0b00001000;
const uint8_t fdi_tcs3471_register_enable_aen = 0b00000010;
const uint8_t fdi_tcs3471_register_enable_pon = 0b00000001;

const uint8_t fdi_tcs3471_register_id = 0x12;
const uint8_t fdi_tcs3471_register_id_tcs34715 = 0x14;

const uint8_t fdi_tcs3471_register_status = 0x13;
const uint8_t fdi_tcs3471_register_status_avalid = 0b00000001;

const uint8_t fdi_tcs3471_register_cdata = 0x14;
const uint8_t fdi_tcs3471_register_cdatah = 0x15;
const uint8_t fdi_tcs3471_register_rdata = 0x16;
const uint8_t fdi_tcs3471_register_rdatah = 0x17;
const uint8_t fdi_tcs3471_register_gdata = 0x18;
const uint8_t fdi_tcs3471_register_gdatah = 0x19;
const uint8_t fdi_tcs3471_register_bdata = 0x1a;
const uint8_t fdi_tcs3471_register_bdatah = 0x1b;

bool fdi_tcs3471_convert(uint8_t address, fdi_tcs3471_conversion_t *conversion) {
    uint8_t command_enable[] = {
        fdi_tcs3471_command | fdi_tcs3471_register_enable,
        fdi_tcs3471_register_enable_wen | fdi_tcs3471_register_enable_aen | fdi_tcs3471_register_enable_pon,
    };
    bool error = fdi_i2c_write(address, command_enable, sizeof(command_enable));
    uint8_t command_status[] = {
        fdi_tcs3471_command | fdi_tcs3471_register_status
    };
    error |= fdi_i2c_write(address, command_status, sizeof(command_status));
    uint8_t status;
    do {
        error |= fdi_i2c_read(address, &status, 1);
    } while ((status & fdi_tcs3471_register_status_avalid) == 0);
    uint8_t command_data[] = {
        fdi_tcs3471_command | fdi_tcs3471_register_cdata
    };
    error |= fdi_i2c_write(address, command_data, sizeof(command_data));
    uint8_t data[8];
    error |= fdi_i2c_read(address, data, sizeof(data));
    conversion->c = (data[1] << 8) | data[0];
    conversion->r = (data[3] << 8) | data[2];
    conversion->g = (data[5] << 8) | data[4];
    conversion->b = (data[7] << 8) | data[6];
    uint8_t command_disable[] = {
        fdi_tcs3471_command | fdi_tcs3471_register_enable,
        0,
    };
    error |= fdi_i2c_write(address, command_disable, sizeof(command_disable));
    return error;
}