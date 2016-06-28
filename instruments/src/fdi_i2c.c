#include "fdi_i2c.h"

#include <stm32f4xx_i2c.h>

#define fdi_i2c_general_call_address 0b00000000

void fdi_i2c_initialize(void) {
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7 /* SCL */| GPIO_Pin_8 /* SDA */;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource8 /* SCL */, GPIO_AF_I2C1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7 /* SDA */, GPIO_AF_I2C1);
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    I2C_InitTypeDef I2C_InitStruct;
    I2C_StructInit(&I2C_InitStruct);
    I2C_InitStruct.I2C_ClockSpeed = 100000;
    I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStruct.I2C_OwnAddress1 = 0x00;
    I2C_InitStruct.I2C_Ack = I2C_Ack_Disable;
    I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C_InitStruct);
    
    I2C_Cmd(I2C1, ENABLE);
}

void fdi_i2c_start(uint8_t address, uint8_t direction) {
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));

    I2C_GenerateSTART(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
            
    I2C_Send7bitAddress(I2C1, address, direction);
      
    uint8_t event = (direction == I2C_Direction_Transmitter) ? I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED : I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED;
    while (!I2C_CheckEvent(I2C1, event));
}

void fdi_i2c_write_byte(uint8_t data) {
    I2C_SendData(I2C1, data);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
}

uint8_t fdi_i2c_read_byte_ack(void) {
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    uint8_t data = I2C_ReceiveData(I2C1);
    return data;
}

uint8_t fdi_i2c_read_byte_nack(void) {
    I2C_AcknowledgeConfig(I2C1, DISABLE);
    I2C_GenerateSTOP(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    uint8_t data = I2C_ReceiveData(I2C1);
    return data;
}

void fdi_i2c_stop(void) {
    I2C_GenerateSTOP(I2C1, ENABLE);
}

bool fdi_i2c_write(uint8_t address, uint8_t* data, int length) {
    fdi_i2c_start(address, I2C_Direction_Transmitter);
    for (int i = 0; i < length; ++i) {
        fdi_i2c_write_byte(data[i]);
    }
    fdi_i2c_stop();
    return true;
}

bool fdi_i2c_read(uint8_t address, uint8_t* data, int length) {
    fdi_i2c_start(address, I2C_Direction_Receiver);
    const int ack_count = length - 1;
    int index = 0;
    while (index < ack_count) {
        data[index++] = fdi_i2c_read_byte_ack();
    }
    data[index] = fdi_i2c_read_byte_nack();
    return true;
}