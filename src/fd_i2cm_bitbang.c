#include "fd_i2cm.h"

#include "fd_delay.h"
#include "fd_gpio.h"

void fd_i2cm_initialize(
    const fd_i2cm_bus_t *buses, uint32_t bus_count,
    const fd_i2cm_device_t *devices, uint32_t device_count
) {
}

void fd_i2cm_clear_bus(const fd_i2cm_bus_t *bus) {
}

void fd_i2cm_bus_enable(const fd_i2cm_bus_t *bus) {
}

void fd_i2cm_bus_disable(const fd_i2cm_bus_t *bus) {
}

bool fd_i2cm_bus_is_enabled(const fd_i2cm_bus_t *bus) {
    return true;
}

#define I2C_SET_SCL() fd_gpio_set(scl, true)
#define I2C_CLR_SCL() fd_gpio_set(scl, false)
#define I2C_SET_SDA() fd_gpio_set(sda, true)
#define I2C_CLR_SDA() fd_gpio_set(sda, false)
#define I2C_DELAY() fd_delay_us(5)

#define i2c_start()\
    I2C_SET_SCL();\
    I2C_SET_SDA();\
    I2C_DELAY();\
    I2C_CLR_SDA();\
    I2C_DELAY();\
    I2C_CLR_SCL();\
    I2C_DELAY()

#define i2c_stop()\
    I2C_CLR_SDA();\
    I2C_DELAY();\
    I2C_SET_SCL();\
    I2C_DELAY();\
    I2C_SET_SDA();\
    I2C_DELAY();

#define i2c_write_bit(b)\
    fd_gpio_set(sda, b);\
    I2C_DELAY();\
    I2C_SET_SCL();\
    I2C_DELAY();\
    I2C_CLR_SCL();

#define i2c_read_bit(b)\
    I2C_SET_SDA();\
    I2C_DELAY();\
    I2C_SET_SCL();\
    I2C_DELAY();\
    b = fd_gpio_get(sda);\
    I2C_CLR_SCL();

bool i2c_write_byte(const fd_i2cm_bus_t *bus, uint8_t byte, bool start, bool stop) {
    const fd_gpio_t scl = bus->scl;
    const fd_gpio_t sda = bus->sda;

    if (start) {
        i2c_start();
    }

    for (int i = 0; i < 8; ++i) {
        i2c_write_bit((byte & 0x80) != 0);
        byte <<= 1;
    }
    
    uint8_t ack;
    i2c_read_bit(ack);

    if (stop) {
        i2c_stop();
    }
    
    return !ack;
}

uint8_t i2c_read_byte(const fd_i2cm_bus_t *bus, bool ack, bool stop) {
    const fd_gpio_t scl = bus->scl;
    const fd_gpio_t sda = bus->sda;

    uint8_t byte = 0;
    for (int i = 0; i < 8; ++i) {
        bool bit;
        i2c_read_bit(bit);
        byte = (byte << 1) | (bit ? 1 : 0);
    }

    i2c_write_bit(!ack);

    if (stop) {
        i2c_stop();
    }

    return byte;
}

bool fd_i2cm_bus_tx(const fd_i2cm_bus_t *bus, const uint8_t *bytes, uint32_t count, bool stop) {
    for (int i = 0; i < count; ++i) {
        bool ack = i2c_write_byte(bus, bytes[i], false, stop && (i == (count - 1)));
        if (!ack) {
            return false;
        }
    }
    return true;
}

void fd_i2cm_bus_rx(const fd_i2cm_bus_t *bus, uint8_t *bytes, uint32_t count, bool stop) {
    for (int i = 0; i < count; ++i) {
        bytes[i] = i2c_read_byte(bus, false, stop && (i == (count - 1)));
    }
}

bool fd_i2cm_device_io(const fd_i2cm_device_t *device, const fd_i2cm_io_t *io) {
    const fd_i2cm_bus_t *bus = device->bus;
    const fd_gpio_t scl = bus->scl;
    const fd_gpio_t sda = bus->sda;

    bool ack = true;
    for (int i = 0; i < io->transfer_count; ++i) {
        bool stop = i == io->transfer_count - 1;
        const fd_i2cm_transfer_t *transfer = &io->transfers[i];
        switch (transfer->direction) {
            case fd_i2cm_direction_tx:
                ack = i2c_write_byte(bus, (device->address << 1) | 1, true, false);
                if (!ack) {
                    goto stop;
                }
                ack = fd_i2cm_bus_tx(bus, transfer->bytes, transfer->byte_count, stop);
                if (!ack) {
                    goto stop;
                }
            break;
            case fd_i2cm_direction_rx:
                ack = i2c_write_byte(bus, device->address << 1, true, false);
                if (!ack) {
                    goto stop;
                }
                fd_i2cm_bus_rx(bus, transfer->bytes, transfer->byte_count, stop);
            break;
            default:
            break;
        }
    }
    
stop:
    i2c_stop();
    return ack;
}

bool fd_i2cm_bus_wait(const fd_i2cm_bus_t *bus) {
    return true;
}