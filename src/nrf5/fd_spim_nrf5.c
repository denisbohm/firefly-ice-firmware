#include "fd_spim.h"

#include "fd_nrf5.h"

static inline
NRF_GPIO_Type *fd_spim_get_nrf_gpio(uint32_t port) {
    return (NRF_GPIO_Type *)(NRF_P0_BASE + port * 0x300UL);
}

static
void fd_spim_configure(
    fd_gpio_t gpio,
    nrf_gpio_pin_dir_t dir,
    nrf_gpio_pin_input_t input,
    nrf_gpio_pin_pull_t pull,
    nrf_gpio_pin_drive_t drive,
    nrf_gpio_pin_sense_t sense
) {
    NRF_GPIO_Type *nrf_gpio = fd_spim_get_nrf_gpio(gpio.port);
    nrf_gpio->PIN_CNF[gpio.pin] = ((uint32_t)dir << GPIO_PIN_CNF_DIR_Pos)
                                | ((uint32_t)input << GPIO_PIN_CNF_INPUT_Pos)
                                | ((uint32_t)pull << GPIO_PIN_CNF_PULL_Pos)
                                | ((uint32_t)drive << GPIO_PIN_CNF_DRIVE_Pos)
                                | ((uint32_t)sense << GPIO_PIN_CNF_SENSE_Pos);
}

void fd_spim_configure_output(fd_gpio_t gpio) {
    fd_spim_configure(gpio,
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_H0H1,
        NRF_GPIO_PIN_NOSENSE
    );
}

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
        fd_spim_configure_output(bus->sclk);
        fd_gpio_set(bus->sclk, true);
        fd_spim_configure_output(bus->mosi);
        fd_gpio_set(bus->mosi, true);
        fd_gpio_configure_input_pull_up(bus->miso);

        fd_spim_bus_disable(bus);
    }
}

void fd_spim_bus_enable(const fd_spim_bus_t *bus) {
    if (fd_spim_bus_is_enabled(bus)) {
        return;
    }

    NRF_SPIM_Type *spim = (NRF_SPIM_Type *)bus->instance;
    switch (bus->frequency) {
        default:
        case 8000000:
            spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M8;
            break;
        case 16000000:
            spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M16;
            break;
        case 32000000:
            spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M32;
            break;
    }
    spim->CONFIG = (SPIM_CONFIG_CPOL_ActiveLow << SPIM_CONFIG_CPOL_Pos) | (SPIM_CONFIG_CPHA_Trailing << SPIM_CONFIG_CPHA_Pos);
    spim->PSEL.SCK = NRF_GPIO_PIN_MAP(bus->sclk.port, bus->sclk.pin);
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
    spim->INTENCLR = SPIM_INTENCLR_END_Msk;
    spim->EVENTS_STARTED = 0;
    spim->EVENTS_STOPPED = 0;
    spim->EVENTS_ENDRX = 0;
    spim->EVENTS_ENDTX = 0;
    spim->EVENTS_END = 0;
    spim->ENABLE = SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos;

    spim->PSEL.SCK = 0xFFFFFFFF;
    spim->PSEL.MOSI = 0xFFFFFFFF;
    spim->PSEL.MISO = 0xFFFFFFFF;
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
        uint32_t tx_amount = tx_remaining > 0xff ? 0xff : tx_remaining;
        spim->TXD.MAXCNT = tx_amount;
        uint32_t rx_amount = rx_remaining > 0xff ? 0xff : rx_remaining;
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