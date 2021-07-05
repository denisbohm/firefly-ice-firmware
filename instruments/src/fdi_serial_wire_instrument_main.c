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
#include "fdi_s25fl116k.h"
#include "fdi_serial_wire.h"
#include "fdi_serial_wire_debug.h"
#include "fdi_spi.h"
#include "fdi_usb.h"

#include "fd_i2cm.h"
#include "fd_log.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static fd_i2cm_bus_t fdi_main_i2cm_buses[] = {
    {
        .instance = 1,
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
        .bus = &fdi_main_i2cm_buses[0],
        .channel = 0,
        .address = 0x30,
    },
    {
        .bus = &fdi_main_i2cm_buses[1],
        .channel = 0,
        .address = 0x32,
    },
};

static fdi_apic_t fdi_main_apics[2];

void main(void) {
    fdi_clock_start_high_speed_internal();

    fdi_gpio_initialize();
    fdi_relay_initialize();
    fdi_spi_initialize();
    fdi_s25fl116k_initialize();
    fdi_serial_wire_initialize();
    fd_i2cm_initialize(
        fdi_main_i2cm_buses, sizeof(fdi_main_i2cm_buses) / sizeof(fdi_main_i2cm_buses[0]),
        fdi_main_i2cm_devices, sizeof(fdi_main_i2cm_devices) / sizeof(fdi_main_i2cm_devices[0])
    );

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

    for (int i = 0; i < 2; ++i) {
        fdi_apic_t *apic = &fdi_main_apics[i];
        const fd_i2cm_device_t *device = &fdi_main_i2cm_devices[i];
        fdi_apic_initialize(apic, device);
        fdi_apic_discover_instruments(apic);
    }

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

    fdi_serial_wire_debug_error_t error = {};
    uint32_t dpid = 0;
    bool success = fdi_serial_wire_debug_connect(serial_wire, &dpid, &error);

    uint32_t address = 0x20000000;
    uint32_t value = 0x5af01234;
    success = fdi_serial_wire_debug_write_memory_uint32(serial_wire, address, value, &error);
    value = 0;
    success = fdi_serial_wire_debug_read_memory_uint32(serial_wire, address, &value, &error);
#endif

    while (true) {
        fdi_api_process();
    }
}
