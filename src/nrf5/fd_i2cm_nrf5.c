#include "fd_i2cm.h"

#include "fd_delay.h"
#include "fd_nrf5.h"

void fd_i2cm_clear_bus(const fd_i2cm_bus_t *bus) {
    fd_gpio_configure_output_open_drain_pull_up(bus->scl);
    fd_gpio_set(bus->scl, true);
    fd_gpio_configure_input_pull_up(bus->sda);

    fd_delay_us(4);
    for (int i = 0; i < 9; i++) {
#if 0
        if (fd_gpio_get(bus->sda)) {
            if (i == 0) {
                return;
            } else {
                break;
            }
        }
#endif
        fd_gpio_set(bus->scl, false);
        fd_delay_us(4);
        fd_gpio_set(bus->scl, true);
        fd_delay_us(4);
    }
    fd_gpio_configure_output_open_drain_pull_up(bus->sda);
    fd_gpio_set(bus->sda, false);
    fd_delay_us(4);
    fd_gpio_set(bus->sda, true);
    fd_delay_us(4);

    fd_gpio_configure_default(bus->scl);
    fd_gpio_configure_default(bus->sda);
}

void fd_i2cm_initialize(
    const fd_i2cm_bus_t *buses, uint32_t bus_count,
    const fd_i2cm_device_t *devices, uint32_t device_count
) {
    for (uint32_t i = 0; i < bus_count; ++i) {
        const fd_i2cm_bus_t *bus = &buses[i];
        fd_i2cm_bus_disable(bus);
        fd_i2cm_clear_bus(bus);
    }
}

void fd_i2cm_bus_enable(const fd_i2cm_bus_t *bus) {
    if (fd_i2cm_bus_is_enabled(bus)) {
        return;
    }

    fd_gpio_configure_output_open_drain_pull_up(bus->scl);
    fd_gpio_set(bus->scl, true);
    fd_gpio_configure_output_open_drain_pull_up(bus->sda);
    fd_gpio_set(bus->sda, true);

    NRF_TWIM_Type *twim = (NRF_TWIM_Type *)bus->instance;

    twim->PSEL.SCL = NRF_GPIO_PIN_MAP(bus->scl.port, bus->scl.pin);
    twim->PSEL.SDA = NRF_GPIO_PIN_MAP(bus->sda.port, bus->sda.pin);

    twim->FREQUENCY = TWIM_FREQUENCY_FREQUENCY_K100 << TWIM_FREQUENCY_FREQUENCY_Pos;
    twim->SHORTS = 0;

    twim->ENABLE = TWIM_ENABLE_ENABLE_Enabled << TWIM_ENABLE_ENABLE_Pos;
}

void fd_i2cm_bus_disable(const fd_i2cm_bus_t *bus) {
    if (!fd_i2cm_bus_is_enabled(bus)) {
        return;
    }

    NRF_TWIM_Type *twim = (NRF_TWIM_Type *)bus->instance;
    twim->ENABLE = TWIM_ENABLE_ENABLE_Disabled << TWIM_ENABLE_ENABLE_Pos;

    twim->PSEL.SCL = 0xFFFFFFFF;
    twim->PSEL.SDA = 0xFFFFFFFF;
}

bool fd_i2cm_bus_is_enabled(const fd_i2cm_bus_t *bus) {
    NRF_TWIM_Type *twim = (NRF_TWIM_Type *)bus->instance;
    const uint32_t mask = TWIM_ENABLE_ENABLE_Enabled << TWIM_ENABLE_ENABLE_Pos;
    return (twim->ENABLE & mask) == mask;
}

bool fd_i2cm_device_io(const fd_i2cm_device_t *device, const fd_i2cm_io_t *io) {
    NRF_TWIM_Type *twim = (NRF_TWIM_Type *)device->bus->instance;
    twim->ADDRESS = device->address;
    twim->EVENTS_ERROR = 0;
    twim->EVENTS_STOPPED = 0;
    twim->ERRORSRC = TWIM_ERRORSRC_ANACK_Msk | TWIM_ERRORSRC_DNACK_Msk | TWIM_ERRORSRC_OVERRUN_Msk;

    uint32_t timeout = device->bus->timeout;
    if (timeout == 0) {
        timeout = UINT32_MAX;
    }
    for (uint32_t i = 0; i < io->transfer_count; ++i) {
        bool last = i == (io->transfer_count - 1);
        twim->EVENTS_SUSPENDED = 0;
        const fd_i2cm_transfer_t *transfer = &io->transfers[i];
        if (transfer->direction == fd_i2cm_direction_tx) {
            twim->TXD.MAXCNT = transfer->byte_count;
            twim->TXD.PTR = (uint32_t)transfer->bytes;
            twim->SHORTS = last ? TWIM_SHORTS_LASTTX_STOP_Msk : TWIM_SHORTS_LASTTX_SUSPEND_Msk;
            twim->TASKS_STARTTX = 1;
        } else {
            twim->RXD.MAXCNT = transfer->byte_count;
            twim->RXD.PTR = (uint32_t)transfer->bytes;
#ifdef NRF52832_XXAA
            // !!! nRF52832 only has shortcut for "last tx suspend" and not "last rx suspend",
            // so that case is not supported on that MCU. An rx can only be the last transfer. -denis
            twim->SHORTS = TWIM_SHORTS_LASTRX_STOP_Msk;
#else // nRF52840
            twim->SHORTS = last ? TWIM_SHORTS_LASTRX_STOP_Msk : TWIM_SHORTS_LASTRX_SUSPEND_Msk;
#endif
            twim->TASKS_STARTRX = 1;
        }
        if (i > 0) {
            twim->TASKS_RESUME = 1;
        }
        uint32_t count = 0;
        if (last) {
            while ((twim->EVENTS_STOPPED == 0) && (twim->EVENTS_ERROR == 0)) {
                if (++count >= timeout) {
                    twim->EVENTS_ERROR = 1;
                    break;
                }
            }
        } else {
            while ((twim->EVENTS_SUSPENDED == 0) && (twim->EVENTS_ERROR == 0)) {
                if (++count >= timeout) {
                    twim->EVENTS_ERROR = 1;
                    break;
                }
            }
        }
        if (twim->EVENTS_ERROR != 0) {
            if (!twim->EVENTS_STOPPED) {
                twim->TASKS_STOP = 1;
                while (twim->EVENTS_STOPPED == 0);
            }
            break;
        }
    }

    return twim->EVENTS_ERROR == 0;
}

bool fd_i2cm_bus_wait(const fd_i2cm_bus_t *bus) {
    return true;
}