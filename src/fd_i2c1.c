#include "fd_i2c1.h"
#include "fd_log.h"
#include "fd_processor.h"

#include <em_cmu.h>
#include <em_gpio.h>
#include <em_i2c.h>

static I2C_TransferReturn_TypeDef i2c1_status;

void fd_i2c1_initialize(void) {
    CMU_ClockEnable(cmuClock_I2C1, true);

    I2C_Init_TypeDef i2c_init = I2C_INIT_DEFAULT;
    i2c_init.freq = I2C_FREQ_FAST_MAX; // 400 KHz
    I2C_Init(I2C1, &i2c_init);

    I2C1->ROUTE = I2C_ROUTE_SDAPEN | I2C_ROUTE_SCLPEN | I2C1_LOCATION;

    I2C_Enable(I2C1, true);
}

void fd_i2c1_sleep(void) {
    I2C1->ROUTE = 0;
    I2C_Enable(I2C1, false);
    CMU_ClockEnable(cmuClock_I2C1, false);
}

void fd_i2c1_wake(void) {
    CMU_ClockEnable(cmuClock_I2C1, true);
    I2C1->ROUTE = I2C_ROUTE_SDAPEN | I2C_ROUTE_SCLPEN | I2C1_LOCATION;
    I2C_Enable(I2C1, true);
}

void fd_i2c1_power_on(void) {
    GPIO_PinModeSet(I2C1_SDA_PORT_PIN, gpioModeWiredAnd, 1);
    GPIO_PinModeSet(I2C1_SCL_PORT_PIN, gpioModeWiredAnd, 1);

    GPIO_PinOutSet(I2C1_PWR_PORT_PIN);
    fd_delay_ms(100); // wait for power to come up (?ms)

    NVIC_ClearPendingIRQ(I2C1_IRQn);
    NVIC_EnableIRQ(I2C1_IRQn);
}

void fd_i2c1_power_off(void) {
    GPIO_PinModeSet(I2C1_SDA_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(I2C1_SCL_PORT_PIN, gpioModePushPull, 0);

    CMU_ClockEnable(cmuClock_I2C1, false);

    GPIO_PinOutClear(I2C1_PWR_PORT_PIN);
}

void I2C1_IRQHandler(void) {
    i2c1_status = I2C_Transfer(I2C1);
}

bool fd_i2c1_sync_transfer(I2C_TransferSeq_TypeDef *seq) {
    i2c1_status = I2C_TransferInit(I2C1, seq);
    while (i2c1_status == i2cTransferInProgress);
    if (i2c1_status != i2cTransferDone) {
        fd_log_ram_assert_fail("");
        return false;
    }
    return true;
}

bool fd_i2c1_register_read(uint8_t device, uint8_t reg, uint8_t *presult) {
    I2C_TransferSeq_TypeDef seq;
    seq.addr = device;
    seq.flags = I2C_FLAG_WRITE_READ;
    seq.buf[0].data = &reg;
    seq.buf[0].len = 1;
    seq.buf[1].data = presult;
    seq.buf[1].len = 1;
    return fd_i2c1_sync_transfer(&seq);
}

bool fd_i2c1_register_write(uint8_t device, uint8_t reg, uint8_t value) {
    I2C_TransferSeq_TypeDef seq;
    seq.addr = device;
    seq.flags = I2C_FLAG_WRITE_WRITE;
    seq.buf[0].data = &reg;
    seq.buf[0].len = 1;
    seq.buf[1].data = &value;
    seq.buf[1].len = 1;
    return fd_i2c1_sync_transfer(&seq);
}

bool fd_i2c1_register_read_bytes(uint8_t device, uint8_t reg, uint8_t *buffer, uint32_t length) {
    I2C_TransferSeq_TypeDef seq;
    seq.addr = device;
    seq.flags = I2C_FLAG_WRITE_READ;
    seq.buf[0].data = &reg;
    seq.buf[0].len = 1;
    seq.buf[1].data = buffer;
    seq.buf[1].len = length;
    return fd_i2c1_sync_transfer(&seq);
}

bool fd_i2c1_register_write_bytes(uint8_t device, uint8_t reg, uint8_t *buffer, uint32_t length) {
    I2C_TransferSeq_TypeDef seq;
    seq.addr = device;
    seq.flags = I2C_FLAG_WRITE_WRITE;
    seq.buf[0].data = &reg;
    seq.buf[0].len = 1;
    seq.buf[1].data = buffer;
    seq.buf[1].len = length;
    return fd_i2c1_sync_transfer(&seq);
}

bool fd_i2c1_read_bytes(uint8_t device, uint16_t address, uint8_t *buffer, uint32_t length) {
    I2C_TransferSeq_TypeDef seq;
    seq.addr = device;
    seq.flags = I2C_FLAG_WRITE_READ;
    seq.buf[0].data = (uint8_t *)&address;
    seq.buf[0].len = 2;
    seq.buf[1].data = buffer;
    seq.buf[1].len = length;
    return fd_i2c1_sync_transfer(&seq);
}

bool fd_i2c1_write_bytes(uint8_t device, uint16_t address, uint8_t *buffer, uint32_t length) {
    I2C_TransferSeq_TypeDef seq;
    seq.addr = device;
    seq.flags = I2C_FLAG_WRITE_WRITE;
    seq.buf[0].data = (uint8_t *)&address;
    seq.buf[0].len = 2;
    seq.buf[1].data = buffer;
    seq.buf[1].len = length;
    bool result = fd_i2c1_sync_transfer(&seq);
    fd_i2c1_poll(device);
    return result;
}

bool fd_i2c1_poll(uint8_t device) {
    I2C_TransferSeq_TypeDef seq;
    seq.addr = device;
    seq.flags = I2C_FLAG_WRITE;
    seq.buf[0].data = 0;
    seq.buf[0].len = 0;
    while (1) {
        i2c1_status = I2C_TransferInit(I2C1, &seq);
        while (i2c1_status == i2cTransferInProgress);
        if (i2c1_status == i2cTransferDone) {
            break;
        } else
        if (i2c1_status == i2cTransferNack) {
            continue;
        } else {
            return false;
        }
    }
    return true;
}