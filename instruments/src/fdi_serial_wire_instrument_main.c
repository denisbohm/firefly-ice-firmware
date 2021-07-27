#include "fdi_main.h"

#include "fdi_instrument.h"

#include "fdi_relay_instrument.h"
#include "fdi_serial_wire_instrument.h"
#include "fdi_storage_instrument.h"

#include "fdi_api.h"
#include "fdi_apic.h"
#include "fdi_clock.h"
#include "fdi_delay.h"
#include "fdi_gpio.h"
#include "fdi_relay.h"
#include "fdi_serial_wire.h"
#include "fdi_serial_wire_debug.h"
#include "fdi_usb.h"

#include "fd_i2cm.h"
#include "fd_log.h"
#include "fd_sdcard.h"
#include "fd_ssd1315.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static fd_i2cm_bus_t fdi_main_i2cm_buses[] = {
    {
        .instance = 1,
        .scl = { .port = 1, .pin = 8 },
        .sda = { .port = 1, .pin = 9 },
        .frequency = 100000,
        .timeout = 1000,
    },
    {
        .instance = 2,
        .scl = { .port = 0, .pin = 9 },
        .sda = { .port = 0, .pin = 8 },
        .frequency = 100000,
        .timeout = 1000,
    },
    {
        .instance = 3,
        .scl = { .port = 2, .pin = 8 },
        .sda = { .port = 2, .pin = 9 },
        .frequency = 100000,
        .timeout = 1000,
    },
};

static fd_i2cm_device_t fdi_main_i2cm_devices[] = {
    {
        .bus = &fdi_main_i2cm_buses[1],
        .channel = 0,
        .address = 0x3c, // display
    },
    {
        .bus = &fdi_main_i2cm_buses[1],
        .channel = 0,
        .address = 0x30, // power board
    },
    {
        .bus = &fdi_main_i2cm_buses[2],
        .channel = 0,
        .address = 0x32, // input output board
    },
};

static fdi_apic_t fdi_main_apics[2];

void main(void) {
    fdi_clock_start_high_speed_internal();

    fdi_gpio_initialize();
    fdi_relay_initialize();

#if 0
    fd_sdcard_spi_initialize();
    fd_sdcard_initialize((fd_sdcard_spi_t) {
        .set_chip_select = fd_sdcard_spi_set_chip_select,
        .set_frequency_slow = fd_sdcard_spi_set_frequency_slow,
        .set_frequency_fast = fd_sdcard_spi_set_frequency_fast,
        .transceive = fd_sdcard_spi_transceive,
    });
#endif

    fdi_serial_wire_initialize();

    fd_i2cm_initialize(
        fdi_main_i2cm_buses, sizeof(fdi_main_i2cm_buses) / sizeof(fdi_main_i2cm_buses[0]),
        fdi_main_i2cm_devices, sizeof(fdi_main_i2cm_devices) / sizeof(fdi_main_i2cm_devices[0])
    );

    fd_ssd1315_initialize(&fdi_main_i2cm_devices[0]);
    fd_ssd1315_write_image_start(0, 0, 128, 64);
    static const uint8_t image_data[8] = { 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f };
    for (int i = 0; i < 128; ++i) {
        fd_ssd1315_write_image_subdata(image_data, sizeof(image_data));
    }
    fd_ssd1315_write_image_end();
    fd_ssd1315_display_on();

    fdi_api_initialize((fdi_api_configuration_t) {
        .can_transmit = fdi_usb_can_send,
        .transmit = fdi_usb_send,
    });
    fdi_usb_initialize();
    fdi_usb_set_data_callback(fdi_api_rx_callback);
    fdi_usb_set_tx_ready_callback(fdi_api_tx_callback);
    fdi_usb_power_up();

    fdi_instrument_initialize();
    fdi_relay_instrument_initialize();
    fdi_serial_wire_instrument_initialize();
    fdi_storage_instrument_initialize();

#if 0
    for (int i = 0; i < 2; ++i) {
        fdi_apic_t *apic = &fdi_main_apics[i];
        const fd_i2cm_device_t *device = &fdi_main_i2cm_devices[i];
        fdi_apic_initialize(apic, device);
        fdi_apic_discover_instruments(apic);
    }
#endif

#if 0
    fdi_serial_wire_t *serial_wire = &fdi_serial_wires[1];
    fdi_serial_wire_reset(serial_wire);
    fdi_serial_wire_set_power(serial_wire, true);

    fdi_serial_wire_set_reset(serial_wire, true);
    fdi_delay_ms(100);
    bool nreset = fdi_serial_wire_get_reset(serial_wire);
    fdi_serial_wire_set_reset(serial_wire, false);
    fdi_delay_ms(100);
    nreset = fdi_serial_wire_get_reset(serial_wire);

    fdi_serial_wire_debug_error_t error = {};
    uint32_t dpid = 0;
    bool success = fdi_serial_wire_debug_connect(serial_wire, &dpid, &error);
    fd_log_assert(dpid == 0x6ba02477);

    uint32_t address = 0x20000000;
    uint32_t value = 0x5af01234;
    success = fdi_serial_wire_debug_write_memory_uint32(serial_wire, address, value, &error);
    value = 0;
    success = fdi_serial_wire_debug_read_memory_uint32(serial_wire, address, &value, &error);
#endif

#if 1
    fdi_serial_wire_t *serial_wire = &fdi_serial_wires[1];
    fdi_serial_wire_reset(serial_wire);
    fdi_serial_wire_set_power(serial_wire, true);

    fdi_serial_wire_set_reset(serial_wire, true);
    fdi_delay_ms(100);
    bool nreset = fdi_serial_wire_get_reset(serial_wire);
    fdi_serial_wire_set_reset(serial_wire, false);
    fdi_delay_ms(100);
    nreset = fdi_serial_wire_get_reset(serial_wire);

    fdi_serial_wire_instrument_t *instrument = fdi_serial_wire_instrument_get_at(1);
    uint32_t dpid;
    fdi_serial_wire_debug_error_t error;
    bool success = fdi_serial_wire_instrument_connect(instrument, &dpid, &error);
    fd_log_assert(success);
    fd_log_assert(dpid == 0x6ba02477);
    uint8_t access_port_id = 0;
    fdi_serial_wire_instrument_set_access_port_id(instrument, access_port_id);
    uint32_t address = 0x20000000;
    uint32_t value = 0x5af01234;
    success = fdi_serial_wire_instrument_write_memory(instrument, address, (uint8_t *)&value, sizeof(value), &error);
    fd_log_assert(success);
    uint32_t verify = 0;
    success = fdi_serial_wire_instrument_read_memory(instrument, address, (uint8_t *)&verify, sizeof(value), &error);
    fd_log_assert(success);
#endif

    while (true) {
        fdi_api_process();
    }
}
