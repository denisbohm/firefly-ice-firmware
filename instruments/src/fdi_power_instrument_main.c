#include "fdi_main.h"

#include "fdi_instrument.h"

#include "fdi_current_instrument.h"
#include "fdi_battery_instrument.h"
#include "fdi_relay_instrument.h"
#include "fdi_voltage_instrument.h"

#include "fdi_adc.h"
#include "fdi_api.h"
#include "fdi_clock.h"
#include "fdi_delay.h"
#include "fdi_gpio.h"
#include "fdi_i2cs.h"
#include "fdi_relay.h"
#include "fdi_usb.h"

#include "fd_log.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

void main(void) {
    fdi_clock_start_high_speed_internal();

    fdi_gpio_initialize();
    fdi_relay_initialize();
    fdi_adc_initialize();
    fdi_adc_power_up();

    fdi_api_initialize((fdi_api_configuration_t) {
        .can_transmit = fdi_i2cs_can_transmit,
        .transmit = fdi_i2cs_transmit,
    });
    fdi_i2cs_initialize((fdi_i2cs_configuration_t) {
        .address = 0x30,
        .rx = fdi_api_rx_callback,
        .tx_ready = fdi_api_tx_callback,
    });

    fdi_instrument_initialize();
    fdi_relay_instrument_initialize();
    fdi_voltage_instrument_initialize();
    fdi_current_instrument_initialize();
    fdi_battery_instrument_initialize();

#if 1
    fdi_relay_instrument_t *relay = fdi_relay_instrument_get_at(0);
    fdi_relay_instrument_set(relay, true);

    fdi_battery_instrument_t *battery_instrument = fdi_battery_instrument_get_at(0);
    fdi_battery_instrument_set_voltage(battery_instrument, 3.8f);
    fdi_battery_instrument_set_enabled(battery_instrument, true);
    float battery_current = fdi_battery_instrument_convert(battery_instrument);

    fdi_voltage_instrument_t *voltage_instrument = fdi_voltage_instrument_get_at(0);
    float battery_voltage = fdi_voltage_instrument_convert(voltage_instrument);

    fdi_current_instrument_t *current_instrument = fdi_current_instrument_get_at(0);
    float usb_current = fdi_current_instrument_convert(current_instrument);
#endif

    while (true) {
        fdi_api_process();
    }
}
