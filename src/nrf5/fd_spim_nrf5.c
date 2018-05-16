#include "fd_spim.h"

#include "fd_nrf5.h"

void fd_spim_initialize(
    const fd_spim_bus_t *buses, uint32_t bus_count,
    const fd_spim_device_t *devices, uint32_t device_count
) {
    for (uint32_t i = 0; i < device_count; ++i) {
        const fd_spim_device_t *device = &devices[i];
        fd_gpio_configure_output(device->csn);
        fd_gpio_set(device->csn, true);
    }
    for (uint32_t i = 0; i < bus_count; ++i) {
        const fd_spim_bus_t *bus = &buses[i];
        fd_gpio_configure_output(bus->sclk);
        fd_gpio_set(bus->sclk, true);
        fd_gpio_configure_output(bus->mosi);
        fd_gpio_set(bus->mosi, true);
        fd_gpio_configure_input(bus->miso);

        fd_spim_bus_disable(bus);
    }
}

void fd_spim_bus_enable(const fd_spim_bus_t *bus) {
    if (fd_spim_bus_is_enabled(bus)) {
        return;
    }

    NRF_SPIM_Type *spim = (NRF_SPIM_Type *)bus->instance;
    spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M8;
    spim->CONFIG = (SPIM_CONFIG_CPOL_ActiveLow << SPIM_CONFIG_CPOL_Pos) | (SPIM_CONFIG_CPHA_Trailing << SPIM_CONFIG_CPHA_Pos);
    spim->PSEL.SCK  = NRF_GPIO_PIN_MAP(bus->sclk.port, bus->sclk.pin);
    spim->PSEL.MOSI = NRF_GPIO_PIN_MAP(bus->mosi.port, bus->mosi.pin);
    spim->PSEL.MISO = NRF_GPIO_PIN_MAP(bus->miso.port, bus->miso.pin);
    spim->TXD.LIST = SPIM_RXD_LIST_LIST_ArrayList << SPIM_RXD_LIST_LIST_Pos;
    spim->RXD.LIST = SPIM_RXD_LIST_LIST_ArrayList << SPIM_RXD_LIST_LIST_Pos;
    spim->ENABLE = SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos;
}

void fd_spim_bus_disable(const fd_spim_bus_t *bus) {
    if (!fd_spim_bus_is_enabled(bus)) {
        return;
    }

    NRF_SPIM_Type *spim = (NRF_SPIM_Type *)bus->instance;
//    NVIC_DisableIRQ(SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn);
    spim->INTENCLR = SPIM_INTENCLR_END_Msk;
    spim->EVENTS_STARTED = 0;
    spim->EVENTS_STOPPED = 0;
    spim->EVENTS_ENDRX = 0;
    spim->EVENTS_ENDTX = 0;
    spim->EVENTS_END = 0;
    spim->ENABLE = SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos;
}

bool fd_spim_bus_is_enabled(const fd_spim_bus_t *bus) {
    NRF_SPIM_Type *spim = (NRF_SPIM_Type *)bus->instance;
    const uint32_t mask = SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos;
    return (spim->ENABLE & mask) == mask;
}

void fd_spim_device_select(const fd_spim_device_t *device) {
    fd_gpio_set(device->csn, false);
}

void fd_spim_device_deselect(const fd_spim_device_t *device) {
    fd_gpio_set(device->csn, true);
}

bool fd_spim_device_is_selected(const fd_spim_device_t *device) {
    return !fd_gpio_get(device->csn);
}

static
void fd_spim_transfer(const fd_spim_bus_t *bus, const uint8_t *tx_bytes, uint32_t tx_byte_count, uint8_t *rx_bytes, uint32_t rx_byte_count) {
    NRF_SPIM_Type *spim = (NRF_SPIM_Type *)bus->instance;
    spim->TXD.PTR = (uint32_t)tx_bytes;
    spim->RXD.PTR = (uint32_t)rx_bytes;
    size_t tx_remaining = tx_byte_count;
    size_t rx_remaining = rx_byte_count;
    while ((tx_remaining > 0) || (rx_remaining > 0)) {
        uint32_t tx_amount = tx_remaining > 0xffff ? 0xffff : tx_remaining;
        spim->TXD.MAXCNT = tx_amount;
        uint32_t rx_amount = rx_remaining > 0xffff ? 0xffff : rx_remaining;
        spim->RXD.MAXCNT = rx_amount;
        spim->EVENTS_END = 0;
        spim->TASKS_START = 1;
        while (!spim->EVENTS_END) {
        }
        tx_remaining -= tx_amount;
        rx_remaining -= rx_amount;
    }
}

void fd_spim_bus_io(const fd_spim_bus_t *bus, const fd_spim_io_t *io) {
    for (uint32_t i = 0; i < io->transfer_count; ++i) {
        fd_spim_transfer_t *transfer = &io->transfers[i];
        fd_spim_transfer(bus, transfer->tx_bytes, transfer->tx_byte_count, transfer->rx_bytes, transfer->rx_byte_count);
    }
}

void fd_spim_bus_wait(const fd_spim_bus_t *bus) {
}