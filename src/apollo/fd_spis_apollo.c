#include "fd_spis.h"

#define fd_assert(e)
#define fd_assertion_failure()
#include "fd_event.h"
#include "fd_gpio.h"
#include "fd_spim.h"

#include "apollo2.h"
#include <am_mcu_apollo.h>

#include <string.h>

const am_hal_ios_config_t fd_spis_ios_config = {
    // Configure the IOS in SPI mode.
    .ui32InterfaceSelect = AM_HAL_IOS_USE_SPI,

    // Eliminate the "read-only" section, so an external host can use the
    // entire "direct write" section.
    .ui32ROBase = 0x78,

    // Making the "FIFO" section as big as possible.
    .ui32FIFOBase = 0x80,

    // We don't need any RAM space, so extend the FIFO all the way to the end
    // of the LRAM.
    .ui32RAMBase = 0x100,

    // FIFO Threshold - set to half the size
    .ui32FIFOThreshold = 0x20,
};

#define AM_IOS_TX_BUFSIZE_MAX   1023
uint8_t g_pui8TxFifoBuffer[AM_IOS_TX_BUFSIZE_MAX];

static uint32_t fd_spis_pin_number(fd_gpio_t gpio) {
    return gpio.port * 32 + gpio.pin;
}

const fd_spis_device_t *fd_spis_devices;
uint32_t fd_spis_device_count;

void fd_spis_initialize(const fd_spis_device_t *devices, uint32_t device_count) {
    fd_spis_devices = devices;
    fd_spis_device_count = device_count;

    fd_assert(device_count <= 1);
    for (uint32_t i = 0; i < device_count; ++i) {
        const fd_spis_device_t *device = &devices[i];
        fd_assert(device->sclk == 0);
        am_hal_gpio_pin_config(fd_spis_pin_number(device->sclk), AM_HAL_PIN_0_SLSCK);
        fd_assert(device->miso == 1);
        am_hal_gpio_pin_config(fd_spis_pin_number(device->miso), AM_HAL_PIN_1_SLMISO);
        fd_assert(device->mosi == 2);
        am_hal_gpio_pin_config(fd_spis_pin_number(device->mosi), AM_HAL_PIN_2_SLMOSI);
        fd_assert(device->csn == 3);
        am_hal_gpio_pin_config(fd_spis_pin_number(device->csn), AM_HAL_PIN_3_SLnCE);

        am_hal_gpio_pin_config(fd_spis_pin_number(device->ready), AM_HAL_GPIO_OUTPUT);

        am_hal_ios_config((am_hal_ios_config_t *)&fd_spis_ios_config);

        //
        // Clear out any IOS register-access interrupts that may be active, and
        // enable interrupts for the registers we're interested in.
        //
        am_hal_ios_access_int_clear(AM_HAL_IOS_ACCESS_INT_ALL);
        am_hal_ios_int_clear(AM_HAL_IOS_INT_ALL);
        am_hal_ios_access_int_enable(AM_HAL_IOS_ACCESS_INT_00);
        am_hal_ios_int_enable(AM_HAL_IOS_INT_XCMPWR | AM_HAL_IOS_INT_XCMPRF);

        // Preparation of FIFO
        am_hal_ios_fifo_buffer_init( &g_pui8TxFifoBuffer[0], AM_IOS_TX_BUFSIZE_MAX);

        //
        // Set the bit in the NVIC to accept access interrupts from the IO Slave.
        //
        am_hal_interrupt_enable(AM_HAL_INTERRUPT_IOSACC);
        am_hal_interrupt_enable(AM_HAL_INTERRUPT_IOSLAVE);

        fd_gpio_configure_output(device->ready);
        fd_gpio_set(device->ready, false);
    }
}

bool fd_spis_device_is_enabled(const fd_spis_device_t *device) {
    bool enabled = AM_REGn(IOSLAVE, device->instance, CFG) & AM_REG_IOSLAVE_CFG_IFCEN(1);
    return enabled;
}

void fd_spis_device_enable(const fd_spis_device_t *device) {
    if (fd_spis_device_is_enabled(device)) {
        return;
    }

    am_hal_ios_enable(device->instance);
    fd_gpio_set(device->ready, true);
}

void fd_spis_device_disable(const fd_spis_device_t *device) {
    if (!fd_spis_device_is_enabled(device)) {
        return;
    }

    fd_gpio_set(device->ready, false);
    am_hal_ios_disable(device->instance);
}

void am_ioslave_acc_isr(void) {
    uint32_t status = am_hal_ios_access_int_status_get(false);
    am_hal_ios_access_int_clear(status);
}

void am_ioslave_ios_isr(void) {
    uint32_t status = am_hal_ios_int_status_get(false);
    am_hal_ios_int_clear(status);

    // host "write to registers" or "read from FIFO" complete 
    if (status & (AM_HAL_IOS_INT_XCMPWR | AM_HAL_IOS_INT_XCMPRF)) {
        const fd_spis_device_t *device = &fd_spis_devices[0];
        if (device->callback) {
            device->callback();
        }
    }
}

size_t fd_spis_get_slave_in_size(const fd_spis_device_t *device __attribute__((unused))) {
    return fd_spis_ios_config.ui32ROBase - 1;
}

size_t fd_spis_get_master_out_size(const fd_spis_device_t *device __attribute__((unused))) {
    return fd_spis_ios_config.ui32RAMBase - fd_spis_ios_config.ui32FIFOBase;
}

void fd_spis_device_before(const fd_spis_device_t *device) {
    fd_gpio_set(device->ready, false);
}

size_t fd_spis_device_slave_in(const fd_spis_device_t *device, uint8_t *data, size_t size) {
    size_t length = am_hal_ios_pui8LRAM[0];
    size_t slave_in_size = fd_spis_get_slave_in_size(device);
    fd_assert(length <= slave_in_size);
    if (length > slave_in_size) {
        length = slave_in_size;
    }
    if (length > size) {
        length = size;
    }
    memcpy(data, (uint8_t *)&am_hal_ios_pui8LRAM[1], length);
    am_hal_ios_pui8LRAM[0] = 0;
    return length;
}

void fd_spis_device_master_out(const fd_spis_device_t *device, const uint8_t *data, size_t length) {
    size_t master_out_size = fd_spis_get_master_out_size(device);
    fd_assert(length <= master_out_size);
    if (length > master_out_size) {
        length = master_out_size;
    }
    memcpy((uint8_t *)&am_hal_ios_pui8LRAM[0x80], data, length);
    volatile uint32_t *IOSLAVE_FIFOPTR = (uint32_t *)(REG_IOSLAVE_BASEADDR + AM_REG_IOSLAVE_FIFOPTR_O);
    uint32_t FIFOSIZ = length;
    uint32_t FIFOPTR = 0;
    *IOSLAVE_FIFOPTR = (FIFOSIZ << 8) | (FIFOPTR << 0);
    volatile uint32_t *IOSLAVE_FIFOCTR = (uint32_t *)(REG_IOSLAVE_BASEADDR + AM_REG_IOSLAVE_FIFOCTR_O);
    *IOSLAVE_FIFOCTR = length;
}

void fd_spis_device_after(const fd_spis_device_t *device) {
    fd_gpio_set(device->ready, true);
}
