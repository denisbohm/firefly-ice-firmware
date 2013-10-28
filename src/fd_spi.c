#include "fd_processor.h"
#include "fd_spi.h"

#include <em_cmu.h>
#include <em_gpio.h>
#include <em_usart.h>

#include <stdbool.h>

// Just for swapped MISO/MOSI workaround -denis
#define FD_SPI_BUS_0 0
#define FD_SPI_BUS_0_SLAVE_W25Q16DW ((FD_SPI_BUS_0 << 16) | 0)

typedef struct {
    USART_InitSync_TypeDef init_sync;

    GPIO_Port_TypeDef csn_port;
    unsigned int csn_pin;
} fd_spi_slave_t;

typedef struct fd_spi_t {
    USART_TypeDef *usart;
    CMU_Clock_TypeDef clock;
    IRQn_Type tx_irqn;
    IRQn_Type rx_irqn;
    uint32_t location;
    GPIO_Port_TypeDef base_port;
    unsigned int base_pin;

    bool pwr_control;
    GPIO_Port_TypeDef pwr_port;
    unsigned int pwr_pin;
    uint32_t pwr_on_duration_ms;

    fd_spi_slave_t *slaves;
    uint32_t slaves_count;

    // temporary variables used during I/O
    fd_spi_slave_t *slave;
    fd_spi_io_t *io;
    //
    bool in_progress;
    bool variable_length;
    uint32_t transfer_index;
    uint32_t transfer_length;
    uint8_t *rx_buffer;
    uint32_t rx_length;
    uint32_t rx_index;
    uint8_t *tx_buffer;
    uint32_t tx_length;
    uint32_t tx_index;
} fd_spi_t;

static fd_spi_t spis[2];
static fd_spi_slave_t spi0_slaves[1];
static fd_spi_slave_t spi1_slaves[2];

void fd_spi_initialize(void) {
    USART_InitSync_TypeDef init_sync_default = USART_INITSYNC_DEFAULT;
    init_sync_default.refFreq = 32000000;
    USART_InitSync_TypeDef init_sync;

    // SPI 0
    fd_spi_t *spi0 = &spis[0];

    spi0->usart = USART0;
    spi0->clock = cmuClock_USART0;
    spi0->tx_irqn = USART0_TX_IRQn;
    spi0->rx_irqn = USART0_RX_IRQn;
    spi0->base_port = gpioPortE;
    spi0->base_pin = 10;
    spi0->location = USART_ROUTE_LOCATION_LOC0;

    spi0->pwr_control = true;
    spi0->pwr_port = gpioPortA;
    spi0->pwr_pin = 3;
    spi0->pwr_on_duration_ms = 10; // W25Q16DW power on time

    // W25Q16DW
    init_sync = init_sync_default;
    init_sync.msbf = true;
    init_sync.clockMode = usartClockMode3;
    init_sync.baudrate = 10000;
    spi0_slaves[0].init_sync = init_sync;
    spi0_slaves[0].csn_port = gpioPortA;
    spi0_slaves[0].csn_pin = 2;

    spi0->slaves = spi0_slaves;
    spi0->slaves_count = sizeof(spi0_slaves) / sizeof(fd_spi_slave_t);

    spi0->slave = &spi0_slaves[0];
    spi0->in_progress = false;

    // SPI 1
    fd_spi_t *spi1 = &spis[1];

    spi1->usart = USART1;
    spi1->clock = cmuClock_USART1;
    spi1->tx_irqn = USART1_TX_IRQn;
    spi1->rx_irqn = USART1_RX_IRQn;
    spi1->base_port = gpioPortD;
    spi1->base_pin = 0;
    spi1->location = USART_ROUTE_LOCATION_LOC1;

    spi1->pwr_control = false;
    spi1->pwr_port = 0;
    spi1->pwr_pin = 0;
    spi1->pwr_on_duration_ms = 0;

    // LIS3DH
    init_sync = init_sync_default;
    init_sync.msbf = true;
    init_sync.clockMode = usartClockMode3;
    init_sync.baudrate = 10000000;
    spi1_slaves[0].init_sync = init_sync;
    spi1_slaves[0].csn_port = gpioPortD;
    spi1_slaves[0].csn_pin = 8;

    // NRF8001
    init_sync = init_sync_default;
    init_sync.msbf = false;
    init_sync.clockMode = usartClockMode0;
    init_sync.baudrate = 1000000;
    spi1_slaves[1].init_sync = init_sync;
    spi1_slaves[1].csn_port = gpioPortD;
    spi1_slaves[1].csn_pin = 3;

    spi1->slaves = spi1_slaves;
    spi1->slaves_count = sizeof(spi1_slaves) / sizeof(fd_spi_slave_t);

    spi1->slave = &spi1_slaves[0];
    spi1->in_progress = false;
}

void fd_spi_on(fd_spi_bus_t bus) {
    fd_spi_t *spi = &spis[bus];

    if (spi->pwr_control) {
        GPIO_PinOutSet(spi->pwr_port, spi->pwr_pin);
    }

    for (uint32_t i = 0; i < spi->slaves_count; ++i) {
        fd_spi_slave_t *slave = &spi->slaves[i];
        GPIO_PinOutSet(slave->csn_port, slave->csn_pin);
    }

    CMU_ClockEnable(spi->clock, true);
    USART_Reset(spi->usart);
    USART_InitSync(spi->usart, &spi->slave->init_sync);
    spi->usart->ROUTE = USART_ROUTE_TXPEN | USART_ROUTE_RXPEN | USART_ROUTE_CLKPEN | spi->location;
    USART_Enable(spi->usart, usartEnable);

    if (spi->pwr_on_duration_ms) {
        fd_delay_ms(spi->pwr_on_duration_ms);
    }
}

void fd_spi_off(fd_spi_bus_t bus) {
    fd_spi_t *spi = &spis[bus];

    // lower tx, rx, clk
    for (int i = 0; i < 3; ++i) {
        GPIO_PinOutClear(spi->base_port, spi->base_pin + i);
    }

    USART_Enable(spi->usart, usartDisable);

    if (spi->pwr_control) {
        for (uint32_t i = 0; i < spi->slaves_count; ++i) {
            fd_spi_slave_t *slave = &spi->slaves[i];
            GPIO_PinOutClear(slave->csn_port, slave->csn_pin);
        }
        GPIO_PinOutClear(spi->pwr_port, spi->pwr_pin);
    }
}

void fd_spi_sleep(fd_spi_bus_t bus) {
    fd_spi_t *spi = &spis[bus];
    CMU_ClockEnable(spi->clock, false);
}

void fd_spi_wake(fd_spi_bus_t bus) {
    fd_spi_t *spi = &spis[bus];
    CMU_ClockEnable(spi->clock, true);
}

static
void start_async_transfer(fd_spi_t *spi);

static
void spi_rx_irq_handler(fd_spi_t *spi) {
    USART_TypeDef *usart = spi->usart;
    if (usart->STATUS & USART_STATUS_RXDATAV) {
        uint8_t rxdata = usart->RXDATA;
        if (spi->rx_index < spi->rx_length) {
            spi->rx_buffer[spi->rx_index++] = rxdata;

            // NRF8001 has a variable length R/W transaction
            fd_spi_io_t *io = spi->io;
            if (spi->variable_length && (spi->rx_index == spi->rx_length)) {
                spi->variable_length = false;
                uint32_t rx_length = spi->rx_index + rxdata;
                fd_spi_transfer_t *transfer = &io->transfers[spi->transfer_index - 1];
                if (rx_length > transfer->rx_size) {
                    rx_length = transfer->rx_size;
                }
                if (rx_length > spi->rx_length) {
                    spi->rx_length = rx_length;
                }
                if (rx_length > spi->transfer_length) {
                    spi->transfer_length = rx_length;
                }
            }
        } else {
            ++spi->rx_index;
        }
        if (spi->rx_index >= spi->transfer_length) {
            fd_spi_io_t *io = spi->io;
            if (spi->transfer_index < io->transfers_count) {
                start_async_transfer(spi);
            } else {
                NVIC_DisableIRQ(spi->tx_irqn);
                usart->IEN &= ~USART_IEN_TXBL;

                NVIC_DisableIRQ(spi->rx_irqn);
                usart->IEN &= ~USART_IEN_RXDATAV;

                spi->in_progress = false;

                if ((io->options & FD_SPI_OPTION_NO_CSN) == 0) {
                    GPIO_PinOutSet(spi->slave->csn_port, spi->slave->csn_pin);
                }

                if (io->completion_callback != 0) {
                    (*io->completion_callback)();
                }
            }
        }
    }
}

static
void spi_tx_irq_handler(fd_spi_t *spi) {
    USART_TypeDef *usart = spi->usart;
    if (usart->STATUS & USART_STATUS_TXBL) {
        if (spi->tx_index < spi->transfer_length) {
            if (spi->tx_index < spi->tx_length) {
                usart->TXDATA = spi->tx_buffer[spi->tx_index];
            } else {
                usart->TXDATA = 0;
            }
            ++spi->tx_index;
        }
    }
}

void USART0_RX_IRQHandler(void) {
    spi_rx_irq_handler(&spis[0]);
}

void USART0_TX_IRQHandler(void) {
    spi_tx_irq_handler(&spis[0]);
}

void USART1_RX_IRQHandler(void) {
    spi_rx_irq_handler(&spis[1]);
}

void USART1_TX_IRQHandler(void) {
    spi_tx_irq_handler(&spis[1]);
}

static
void start_async_transfer(fd_spi_t *spi) {
    fd_spi_transfer_t *transfer = &spi->io->transfers[spi->transfer_index++];

    spi->transfer_length = 0;
    if (transfer->op & fd_spi_op_read) {
        if (transfer->rx_length > spi->transfer_length) {
            spi->transfer_length = transfer->rx_length;
        }
    }
    if (transfer->op & fd_spi_op_write) {
        if (transfer->tx_length > spi->transfer_length) {
            spi->transfer_length = transfer->tx_length;
        }
    }

    spi->rx_buffer = transfer->rx_buffer;
    spi->rx_length = transfer->rx_length;
    spi->rx_index = 0;
    spi->usart->CMD = USART_CMD_CLEARRX;
    NVIC_ClearPendingIRQ(spi->rx_irqn);
//    NVIC_EnableIRQ(spi->rx_irqn);
    spi->usart->IEN |= USART_IEN_RXDATAV;

    spi->tx_buffer = transfer->tx_buffer;
    spi->tx_length = transfer->tx_length;
    spi->tx_index = 0;
    spi->usart->CMD = USART_CMD_CLEARTX;
    NVIC_ClearPendingIRQ(spi->tx_irqn);
//    NVIC_EnableIRQ(spi->tx_irqn);
    spi->usart->IEN |= USART_IEN_TXBL;
}

void fd_spi_set_device(fd_spi_device_t device) {
    uint32_t bus = device >> 16;
    fd_spi_t *spi = &spis[bus];
    fd_spi_slave_t *slave = &spi->slaves[device & 0xffff];
    if (slave != spi->slave) {
        spi->slave = slave;
        USART_Reset(spi->usart);
        USART_InitSync(spi->usart, &slave->init_sync);
        spi->usart->ROUTE = USART_ROUTE_TXPEN | USART_ROUTE_RXPEN | USART_ROUTE_CLKPEN | spi->location;
    }
}

void fd_spi_io(fd_spi_device_t device, fd_spi_io_t *io) {
    fd_spi_set_device(device);

    uint32_t bus = device >> 16;
    fd_spi_t *spi = &spis[bus];
    spi->io = io;
    spi->in_progress = true;
    spi->variable_length = io->options & FD_SPI_OPTION_VARLEN ? true : false;
    spi->transfer_index = 0;

    if ((io->options & FD_SPI_OPTION_NO_CSN) == 0) {
        GPIO_PinOutClear(spi->slave->csn_port, spi->slave->csn_pin);
    }

    start_async_transfer(spi);
}

void fd_spi_wait(fd_spi_bus_t bus) {
    fd_spi_t *spi = &spis[bus];
    while (spi->in_progress) {
//        spi_rx_irq_handler(&spis[0]);
//        spi_tx_irq_handler(&spis[0]);
        spi_rx_irq_handler(&spis[1]);
        spi_tx_irq_handler(&spis[1]);
    }
}

void fd_spi_chip_select(fd_spi_device_t device, bool select) {
    uint32_t bus = device >> 16;
    fd_spi_t *spi = &spis[bus];
    fd_spi_slave_t *slave = &spi->slaves[device & 0xffff];
    if (select) {
        GPIO_PinOutClear(slave->csn_port, slave->csn_pin);
    } else {
        GPIO_PinOutSet(slave->csn_port, slave->csn_pin);
    }
}

void fd_spi_sync_tx1(fd_spi_device_t device, uint8_t byte) {
    fd_spi_sync_txn(device, &byte, 1);
}

void fd_spi_sync_tx2(fd_spi_device_t device, uint8_t byte0, uint8_t byte1) {
    uint8_t bytes[] = {byte0, byte1};
    fd_spi_sync_txn(device, bytes, sizeof(bytes));
}

void fd_spi_sync_txn(fd_spi_device_t device, uint8_t *bytes, uint32_t length) {
    fd_spi_transfer_t transfers[] = {
        {
            .op = fd_spi_op_write,
            .tx_buffer = bytes,
            .tx_size = length,
            .tx_length = length,
            .rx_buffer = 0,
            .rx_size = 0,
            .rx_length = 0,
        },
    };
    fd_spi_io_t io = {
        .options = 0,
        .transfers = transfers,
        .transfers_count = sizeof(transfers) / sizeof(fd_spi_transfer_t),
        .completion_callback = 0,
    };
    fd_spi_io(device, &io);
    uint32_t bus = device >> 16;
    fd_spi_wait(bus);
}

void fd_spi_sync_txn_rxn(
    fd_spi_device_t device,
    uint8_t* tx_bytes, uint32_t tx_length,
    uint8_t* rx_bytes, uint32_t rx_length
) {
    fd_spi_transfer_t transfers[] = {
        {
            .op = fd_spi_op_write,
            .tx_buffer = tx_bytes,
            .tx_size = tx_length,
            .tx_length = tx_length,
            .rx_buffer = 0,
            .rx_size = 0,
            .rx_length = 0,
        },
        {
            .op = fd_spi_op_read,
            .tx_buffer = 0,
            .tx_size = 0,
            .tx_length = 0,
            .rx_buffer = rx_bytes,
            .rx_size = rx_length,
            .rx_length = rx_length,
        },
    };
    fd_spi_io_t io = {
        .options = 0,
        .transfers = transfers,
        .transfers_count = sizeof(transfers) / sizeof(fd_spi_transfer_t),
        .completion_callback = 0,
    };
    fd_spi_io(device, &io);
    uint32_t bus = device >> 16;
    fd_spi_wait(bus);
}

uint8_t fd_spi_sync_tx1_rx1(fd_spi_device_t device, uint8_t byte) {
    uint8_t result;
    fd_spi_sync_txn_rxn(device, &byte, 1, &result, 1);
    return result;
}