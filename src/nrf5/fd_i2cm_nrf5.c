#include "fd_i2cm.h"

#include "fd_nrf5.h"

void fd_i2cm_initialize(
    const fd_i2cm_bus_t *buses, uint32_t bus_count,
    const fd_i2cm_device_t *devices, uint32_t device_count
) {
    for (uint32_t i = 0; i < bus_count; ++i) {
        const fd_i2cm_bus_t *bus = &buses[i];
        fd_i2cm_bus_disable(bus);
    }
}

void fd_i2cm_bus_enable(const fd_i2cm_bus_t *bus) {
    if (fd_i2cm_bus_is_enabled(bus)) {
        return;
    }

    fd_gpio_configure_output_open_drain(bus->scl);
    fd_gpio_set(bus->scl, true);
    fd_gpio_configure_output_open_drain(bus->sda);
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
            twim->SHORTS = last ? TWIM_SHORTS_LASTRX_STOP_Msk : TWIM_SHORTS_LASTRX_SUSPEND_Msk;
            twim->TASKS_STARTRX = 1;
        }
        if (last) {
            while ((twim->EVENTS_STOPPED == 0) && (twim->EVENTS_ERROR == 0));
        } else {
            while ((twim->EVENTS_SUSPENDED == 0) && (twim->EVENTS_ERROR == 0));
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