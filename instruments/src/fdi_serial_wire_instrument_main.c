#include "fdi_main.h"

#include "fdi_instrument.h"

#include "fdi_indicator_instrument.h"
#include "fdi_serial_wire_instrument.h"
#include "fdi_storage_instrument.h"
#include "fdi_voltage_instrument.h"

#include "fdi_api.h"
#include "fdi_adc.h"
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
#include "fd_timing.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static fd_i2cm_bus_t fdi_main_i2cm_buses[] = {
    {
        .instance = 2,
        .scl = { .port = 0, .pin = 9 },
        .sda = { .port = 0, .pin = 8 },
        .pullup = false,
        .frequency = 100000,
        .timeout = 1000,
    },
    {
        .instance = 3,
        .scl = { .port = 2, .pin = 8 },
        .sda = { .port = 2, .pin = 9 },
        .pullup = false,
        .frequency = 100000,
        .timeout = 1000,
    },
    {
        .instance = 1,
        .scl = { .port = 1, .pin = 8 },
        .sda = { .port = 1, .pin = 9 },
        .pullup = true,
        .frequency = 100000,
        .timeout = 1000,
    },
};

static fd_i2cm_device_t fdi_main_i2cm_devices[] = {
    {
        .bus = &fdi_main_i2cm_buses[0],
        .channel = 0,
        .address = 0x30, // power board
    },
    {
        .bus = &fdi_main_i2cm_buses[1],
        .channel = 0,
        .address = 0x32, // input output board
    },
    {
        .bus = &fdi_main_i2cm_buses[2],
        .channel = 0,
        .address = 0x3c, // display
    },
};

static fdi_apic_t fdi_main_apics[2];

bool fdi_main_usb_transmit(uint8_t *data, size_t length) {
    if (!fdi_usb_can_send()) {
        return false;
    }
    fdi_usb_send(data, length);
    return true;
}

void main(void) {
    fdi_clock_start_high_speed_internal();

    fdi_gpio_initialize();
    fdi_relay_initialize();
    fdi_adc_initialize();
    fdi_adc_power_up();

    fd_sdcard_spi_initialize();
    fd_sdcard_initialize((fd_sdcard_spi_t) {
        .set_chip_select = fd_sdcard_spi_set_chip_select,
        .set_frequency_slow = fd_sdcard_spi_set_frequency_slow,
        .set_frequency_fast = fd_sdcard_spi_set_frequency_fast,
        .transceive = fd_sdcard_spi_transceive,
    });

#if 0
    uint32_t address = 0;
    uint8_t data[] = { 0xf0 };
    fd_sdcard_write(address, data, sizeof(data));
    uint8_t verify[sizeof(data)] = { 0 };
    fd_sdcard_read(address, verify, sizeof(verify));
    fd_log_assert(memcmp(verify, data, sizeof(data)) == 0);
#endif

    fdi_serial_wire_initialize();

    fd_i2cm_initialize(
        fdi_main_i2cm_buses, sizeof(fdi_main_i2cm_buses) / sizeof(fdi_main_i2cm_buses[0]),
        fdi_main_i2cm_devices, sizeof(fdi_main_i2cm_devices) / sizeof(fdi_main_i2cm_devices[0])
    );

    fd_ssd1315_initialize(&fdi_main_i2cm_devices[2]);
    fd_ssd1315_write_image_start(0, 0, 128, 64);
    static const uint8_t image_data[8] = { 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f };
    for (int i = 0; i < 128; ++i) {
        fd_ssd1315_write_image_subdata(image_data, sizeof(image_data));
    }
    fd_ssd1315_write_image_end();
    fd_ssd1315_display_on();

    uint32_t apic_count = sizeof(fdi_main_apics) / sizeof(fdi_main_apics[0]);
    for (uint32_t i = 0; i < apic_count; ++i) {
        fdi_apic_initialize(&fdi_main_apics[i], &fdi_main_i2cm_devices[i]);
    }

    fdi_api_initialize((fdi_api_configuration_t) {
        .transmit = fdi_main_usb_transmit,
        .apic_count = apic_count,
        .apics = fdi_main_apics,
    });
    fdi_usb_initialize();
    fdi_usb_set_data_callback(fdi_api_rx_callback);
    fdi_usb_set_tx_ready_callback(fdi_api_tx_callback);
    fdi_usb_power_up();

    fdi_instrument_initialize();
    fdi_indicator_instrument_initialize();
    fdi_serial_wire_instrument_initialize();
    fdi_storage_instrument_initialize();
    fdi_voltage_instrument_initialize();

    uint32_t retries = 0;
    for (uint32_t i = 0; i < apic_count; ++i) {
        // wait for instrument board to power up and start responding
        fdi_apic_t *apic = &fdi_main_apics[i];
        uint8_t data[] = { 'e', 'c', 'h', 'o' };
        for (uint32_t j = 0; j < 100; ++j) {
            if (fdi_apic_echo(apic, data, sizeof(data))) {
                break;
            }
            ++retries;
            fdi_delay_ms(100);
        }

        fdi_apic_discover_instruments(apic);
    }

#if 0
    fdi_apic_t *power_apic = &fdi_main_apics[0];
    fdi_apic_instrument_t *relay_vusb_to_dut = fdi_apic_get_instrument(power_apic, 41);
    // relay_vusb_to_dut.set(true);
    const uint8_t content[] = { 1 };
    bool ok = fdi_apic_write(power_apic, relay_vusb_to_dut->identifier, 1, content, sizeof(content));
    fd_log_assert(ok);

    fdi_serial_wire_t *serial_wire = &fdi_serial_wires[1];
    fdi_serial_wire_reset(serial_wire);
    fdi_serial_wire_set_power(serial_wire, true);

    fdi_delay_ms(1000);

    fdi_voltage_instrument_t *voltage_instrument = fdi_voltage_instrument_get_at(1);
    float voltage = fdi_voltage_instrument_convert(voltage_instrument);
    fd_log_assert(voltage > 1.7f);

    fdi_serial_wire_set_reset(serial_wire, true);
    fdi_delay_ms(100);
    bool nreset = fdi_serial_wire_get_reset(serial_wire);
    fdi_serial_wire_set_reset(serial_wire, false);
    fdi_delay_ms(100);
    nreset = fdi_serial_wire_get_reset(serial_wire);

    fdi_serial_wire_debug_error_t error = {};
    uint32_t dpid = 0;
    bool success = fdi_serial_wire_debug_connect(serial_wire, &dpid, &error);
    fd_log_assert(success);
    fd_log_assert(dpid == 0x6ba02477);

    fdi_serial_wire_debug_select_access_port_id(serial_wire, 1); // ahb net
    uint32_t ahb_app_idr;
    success = fdi_serial_wire_debug_select_and_read_access_port(
        serial_wire, SWD_AP_IDR, &ahb_app_idr, &error
    );
    fd_log_assert(success);
    fd_log_assert(ahb_app_idr == 0x84770001);

    // erase all app and net
    for (int ap = 2; ap <= 3; ++ap) {
        fdi_serial_wire_debug_select_access_port_id(serial_wire, ap);
        uint32_t ctrl_app_idr;
        success = fdi_serial_wire_debug_select_and_read_access_port(
            serial_wire, SWD_AP_IDR, &ctrl_app_idr, &error
        );
        fd_log_assert(success);
        fd_log_assert(ctrl_app_idr == 0x12880000);

        success = fdi_serial_wire_debug_select_and_write_access_port(
            serial_wire, 0x04 /* erase all */, 1 /* erase */, &error
        );
        fd_log_assert(success);
        uint32_t ctrl_app_erase_all_status;
        success = fdi_serial_wire_debug_select_and_read_access_port(
            serial_wire, 0x08 /* erase all status */, &ctrl_app_erase_all_status, &error
        );
        fd_log_assert(success);
        fd_log_assert(ctrl_app_erase_all_status == 0x00000001);
        fdi_delay_ms(500);
        success = fdi_serial_wire_debug_select_and_read_access_port(
            serial_wire, 0x08 /* erase all status */, &ctrl_app_erase_all_status, &error
        );
        fd_log_assert(success);
        fd_log_assert(ctrl_app_erase_all_status == 0x00000000);
    }

    fdi_serial_wire_debug_select_access_port_id(serial_wire, 0); // ahb app

    success = fdi_serial_wire_debug_select_and_read_access_port(
        serial_wire, SWD_AP_IDR, &ahb_app_idr, &error
    );
    fd_log_assert(success);
    fd_log_assert(ahb_app_idr == 0x84770001);

    success = fdi_serial_wire_debug_select_and_write_access_port(
        serial_wire, SWD_AP_CSW, SWD_AP_CSW_PROT | SWD_AP_CSW_ADDRINC_SINGLE | SWD_AP_CSW_SIZE_32BIT, &error
    );
    fd_log_assert(success);

    success = fdi_serial_wire_debug_halt(serial_wire, &error);
    fd_log_assert(success);
    bool halted = false;
    success = fdi_serial_wire_debug_is_halted(serial_wire, &halted, &error);
    fd_log_assert(success);
    fd_log_assert(halted);

    uint32_t reset_network_forceoff = 0;
    success = fdi_serial_wire_debug_read_memory_uint32(serial_wire, 0x50005614, &reset_network_forceoff, &error);
    fd_log_assert(success);
    success = fdi_serial_wire_debug_write_memory_uint32(serial_wire, 0x50005614, 0, &error);
    fd_log_assert(success);
    success = fdi_serial_wire_debug_read_memory_uint32(serial_wire, 0x50005614, &reset_network_forceoff, &error);
    fd_log_assert(success);

    fdi_serial_wire_debug_select_access_port_id(serial_wire, 1); // ahb net
    success = fdi_serial_wire_debug_select_and_read_access_port(
        serial_wire, SWD_AP_IDR, &ahb_app_idr, &error
    );
    fd_log_assert(success);
    fd_log_assert(ahb_app_idr == 0x84770001);

    success = fdi_serial_wire_debug_select_and_write_access_port(
        serial_wire, SWD_AP_CSW, SWD_AP_CSW_PROT | SWD_AP_CSW_ADDRINC_SINGLE | SWD_AP_CSW_SIZE_32BIT, &error
    );
    fd_log_assert(success);

    success = fdi_serial_wire_debug_halt(serial_wire, &error);
    fd_log_assert(success);
    halted = false;
    success = fdi_serial_wire_debug_is_halted(serial_wire, &halted, &error);
    fd_log_assert(success);
    fd_log_assert(halted);

    uint32_t erased = 0;
    success = fdi_serial_wire_debug_read_memory_uint32(serial_wire, 0x01000000, &erased, &error);
    fd_log_assert(success);
    fd_log_assert(erased == 0xffffffff);

    uint32_t address = 0x21000000;
    uint32_t value = 0x5af01234;
    success = fdi_serial_wire_debug_write_memory_uint32(serial_wire, address, value, &error);
    fd_log_assert(success);
    uint32_t verify = 0;
    success = fdi_serial_wire_debug_read_memory_uint32(serial_wire, address, &verify, &error);
    fd_log_assert(success);
    fd_log_assert(verify == value);

    success = fdi_serial_wire_debug_write_data(serial_wire, address, (uint8_t *)&value, sizeof(value), &error);
    fd_log_assert(success);
    uint32_t data = 0;
    success = fdi_serial_wire_debug_read_data(serial_wire, address, (uint8_t *)&data, sizeof(data), &error);
    fd_log_assert(success);
    fd_log_assert(data == value);

    uint32_t r0 = 0;
    success = fdi_serial_wire_debug_write_register(serial_wire, 0, r0, &error);
    fd_log_assert(success);
    uint32_t r0_verify = 0;
    success = fdi_serial_wire_debug_read_register(serial_wire, 0, &r0_verify, &error);
    fd_log_assert(success);
#endif

    while (true) {
        fdi_api_process();
    }
}
