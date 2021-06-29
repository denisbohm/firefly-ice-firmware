#include "fdi_smbus.h"

#include <stm32f4xx_i2c.h>

#define GP  0x107   /* x^8 + x^2 + x + 1 */
#define DI  0x07

static uint8_t crc8_table[256];

void fdi_smbus_initialize(void) {
    for (int i = 0; i < 256; ++i) {
        uint8_t crc = i;
        for (int j = 0; j < 8; ++j) {
            crc = (crc << 1) ^ ((crc & 0x80) ? DI : 0);
        }
        crc8_table[i] = crc & 0xFF;
    }
}

static
uint8_t fdi_smbus_crc8_initialize(void) {
     return 0;
}

static
void fdi_smbus_crc8_update(uint8_t *crc, uint8_t byte) {
   *crc = crc8_table[(*crc) ^ byte];
}

#define fdi_smbus_max_count 10000

#define fdi_smbus_while_not_event(event) do {\
    uint32_t counter = fdi_smbus_max_count;\
    while (!I2C_CheckEvent(I2C2, event) && counter) --counter;\
    if (counter == 0) {\
	return false;\
    }\
} while (false)

bool fdi_smbus_write(uint8_t device_address, uint8_t register_address, uint8_t *data, uint32_t length) {
    I2C_GenerateSTART(I2C2, ENABLE);
    fdi_smbus_while_not_event(I2C_EVENT_MASTER_MODE_SELECT);

    I2C_Send7bitAddress(I2C2, device_address, I2C_Direction_Transmitter);
    fdi_smbus_while_not_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);

    uint8_t pec = fdi_smbus_crc8_initialize();
    I2C_SendData(I2C2, register_address);
    fdi_smbus_while_not_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED);
    fdi_smbus_crc8_update(&pec, register_address);

    for (uint32_t i = 0; i < length; ++i) {
        uint8_t byte = data[i];
        I2C_SendData(I2C2, byte);
        fdi_smbus_while_not_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED);
        fdi_smbus_crc8_update(&pec, byte);
    }

    I2C_SendData(I2C2, pec);
    fdi_smbus_while_not_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED);

    I2C_GenerateSTOP(I2C2, ENABLE);
    return true;
}

bool fdi_smbus_read(uint8_t device_address, uint8_t register_address, uint8_t *data, uint32_t length) {
    I2C_GenerateSTART(I2C2, ENABLE);
    fdi_smbus_while_not_event(I2C_EVENT_MASTER_MODE_SELECT);

    I2C_Send7bitAddress(I2C2, device_address, I2C_Direction_Transmitter);
    fdi_smbus_while_not_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);

    I2C_Cmd(I2C2, ENABLE);

    I2C_SendData(I2C2, register_address);
    fdi_smbus_while_not_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED);

    I2C_GenerateSTART(I2C2, ENABLE);
    fdi_smbus_while_not_event(I2C_EVENT_MASTER_MODE_SELECT);

    I2C_Send7bitAddress(I2C2, device_address, I2C_Direction_Receiver);
    fdi_smbus_while_not_event(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED);
    uint8_t pec = fdi_smbus_crc8_initialize();
    fdi_smbus_crc8_update(&pec, register_address);

    for (uint32_t i = 0; i < length; ++i) {
        fdi_smbus_while_not_event(I2C_EVENT_MASTER_BYTE_RECEIVED);
        data[i] = I2C_ReceiveData(I2C2);
    }

    fdi_smbus_while_not_event(I2C_EVENT_MASTER_BYTE_RECEIVED);
    uint8_t pec_expected = I2C_ReceiveData(I2C2);
    I2C_GenerateSTOP(I2C2, ENABLE);

    if (pec != pec_expected) {
        return false;
    }

    return true;
}