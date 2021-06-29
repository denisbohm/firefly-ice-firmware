#include "fdi_main.h"

#include "fdi_instrument.h"

#include "fdi_relay_instrument.h"
#include "fdi_serial_wire_instrument.h"
#include "fdi_storage_instrument.h"

#include "fdi_api.h"
#include "fdi_clock.h"
#include "fdi_delay.h"
#include "fdi_gpio.h"
#include "fdi_relay.h"
#include "fdi_s25fl116k.h"
#include "fdi_serial_wire.h"
#include "fdi_serial_wire_debug.h"
#include "fdi_spi.h"
#include "fdi_usb.h"

#include "fd_log.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

void main(void) {
    fdi_clock_start_high_speed_internal();

    fdi_api_initialize();

    fdi_gpio_initialize();
    fdi_relay_initialize();
    fdi_spi_initialize();
    fdi_s25fl116k_initialize();
    fdi_serial_wire_initialize();

    fdi_usb_initialize();
    fdi_api_initialize_usb();
    fdi_usb_power_up();

    fdi_instrument_initialize();
    fdi_relay_instrument_initialize();
    fdi_serial_wire_instrument_initialize();
    fdi_storage_instrument_initialize();

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
