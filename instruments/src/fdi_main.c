#include "fdi_main.h"

#include "fdi_instrument.h"

#include "fdi_battery_instrument.h"
#include "fdi_color_instrument.h"
#include "fdi_current_instrument.h"
#include "fdi_indicator_instrument.h"
#include "fdi_relay_instrument.h"
#include "fdi_serial_wire_instrument.h"
#include "fdi_voltage_instrument.h"

#include "fdi_adc.h"
#include "fdi_api.h"
#include "fdi_clock.h"
#include "fdi_delay.h"
#include "fdi_gpio.h"
#include "fdi_i2c.h"
#include "fdi_mcp4726.h"
#include "fdi_relay.h"
#include "fdi_serial_wire.h"
#include "fdi_tcs3471.h"
#include "fdi_usb.h"

#include "fd_log.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

void fdi_main(void) {
    fdi_clock_start_high_speed_internal();

    fdi_api_initialize();

    fdi_gpio_initialize();
    fdi_relay_initialize();
    fdi_i2c_initialize();
    fdi_serial_wire_initialize();
    fdi_adc_initialize();

    fdi_usb_initialize();
    fdi_api_initialize_usb();
    fdi_usb_power_up();

    fdi_instrument_initialize();
    fdi_battery_instrument_initialize();
    fdi_color_instrument_initialize();
    fdi_current_instrument_initialize();
    fdi_indicator_instrument_initialize();
    fdi_relay_instrument_initialize();
    fdi_serial_wire_instrument_initialize();
    fdi_voltage_instrument_initialize();

    while (true) {
        fdi_api_process();
    }
}
