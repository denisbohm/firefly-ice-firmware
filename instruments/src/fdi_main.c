#include "fdi_main.h"

#include "fdi_instrument.h"

#include "fdi_battery_instrument.h"
#include "fdi_color_instrument.h"
#include "fdi_current_instrument.h"
#include "fdi_indicator_instrument.h"
#include "fdi_relay_instrument.h"
#include "fdi_serial_wire_instrument.h"
#include "fdi_storage_instrument.h"
#include "fdi_voltage_instrument.h"

#include "fdi_adc.h"
#include "fdi_api.h"
#include "fdi_clock.h"
#include "fdi_delay.h"
#include "fdi_gpio.h"
#include "fdi_i2c.h"
#include "fdi_mcp4726.h"
#include "fdi_relay.h"
#include "fdi_s25fl116k.h"
#include "fdi_serial_wire.h"
#include "fdi_serial_wire_debug.h"
#include "fdi_spi.h"
#include "fdi_tcs3471.h"
#include "fdi_usb.h"

#include "fd_log.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

void fdi_main(void) {
    fdi_clock_start_high_speed_internal();

    fdi_gpio_initialize();
    fdi_relay_initialize();
    fdi_i2c_initialize();
    fdi_spi_initialize();
    fdi_s25fl116k_initialize();
    fdi_serial_wire_initialize();
    fdi_adc_initialize();
    fdi_adc_power_up();

    fdi_api_initialize((fdi_api_configuration_t) {
        .can_transmit = fdi_usb_can_send,
        .transmit = fdi_usb_send,
    });
    fdi_usb_initialize();
    fdi_usb_set_data_callback(fdi_api_rx_callback);
    fdi_usb_set_tx_ready_callback(fdi_api_tx_callback);
    fdi_usb_power_up();

    fdi_instrument_initialize();
    fdi_battery_instrument_initialize();
    fdi_color_instrument_initialize();
    fdi_current_instrument_initialize();
    fdi_indicator_instrument_initialize();
    fdi_relay_instrument_initialize();
    fdi_serial_wire_instrument_initialize();
    fdi_storage_instrument_initialize();
    fdi_voltage_instrument_initialize();

#if 0
    fdi_battery_instrument_t *instrument = fdi_battery_instrument_get_at(0);
    fdi_battery_instrument_set_voltage(instrument, 3.8);
    fdi_battery_instrument_set_enabled(instrument, true);

    fdi_delay_ms(500);

    fdi_serial_wire_t *serial_wire = &fdi_serial_wires[0];
    fdi_serial_wire_reset(serial_wire);
    fdi_serial_wire_set_power(serial_wire, true);

    fdi_serial_wire_set_reset(serial_wire, false);
    fdi_delay_ms(500);
    fdi_serial_wire_set_reset(serial_wire, true);

    fdi_delay_ms(500);

    fdi_serial_wire_debug_test(serial_wire);
#endif

    while (true) {
        fdi_api_process();
        fdi_battery_instrument_save_conversions();
    }
}
