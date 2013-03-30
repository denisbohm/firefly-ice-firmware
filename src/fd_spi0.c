#include "fd_processor.h"
#include "fd_spi0.h"

#include <em_cmu.h>
#include <em_usart.h>

static fd_spi_transfer *async_transfers;
static uint32_t async_count;
static void (*async_callback)(void);
static uint32_t async_index;
static uint32_t async_flags;
#define rx_complete_flag 0x01
#define tx_complete_flag 0x02
#define complete_flag 0x04

static uint8_t *rx_buffer;
static uint32_t rx_length;
static uint32_t rx_index;
static void (*rx_callback)(void);

static uint8_t *tx_buffer;
static uint32_t tx_length;
static uint32_t tx_index;
static void (*tx_callback)(void);

void fd_spi0_initialize(void) {
    CMU_ClockEnable(cmuClock_USART0, true);

    USART_InitSync_TypeDef spi_setup = USART_INITSYNC_DEFAULT;
    spi_setup.msbf = true;
    spi_setup.clockMode = usartClockMode3;
    spi_setup.baudrate = 10000000;
    USART_InitSync(USART0, &spi_setup);

    USART0->ROUTE = USART_ROUTE_TXPEN | USART_ROUTE_RXPEN | USART_ROUTE_CLKPEN | US0_LOCATION;

    USART_Enable(USART0, usartEnable);
}

void USART0_RX_IRQHandler(void) {
    if (USART0->STATUS & USART_STATUS_RXDATAV) {
        uint8_t rxdata = USART0->RXDATA;
        if (rx_index < rx_length) {
            rx_buffer[rx_index++] = rxdata;
        } else {
            NVIC_DisableIRQ(USART0_RX_IRQn);
            USART0->IEN &= ~USART_IEN_RXDATAV;
            if (rx_callback != 0) {
                (*rx_callback)();
            }
        }
    }
}

void USART0_TX_IRQHandler(void) {
    if (USART0->STATUS & USART_STATUS_TXBL) {
        if (tx_index < tx_length) {
            USART0->TXDATA = tx_buffer[tx_index++];
        } else {
            NVIC_DisableIRQ(USART0_TX_IRQn);
            USART0->IEN &= ~USART_IEN_TXBL;
            if (tx_callback != 0) {
                (*tx_callback)();
            }
        }
    }
}

static
void start_async_rx(uint8_t *buffer, uint32_t length, void (*callback)(void)) {
    rx_buffer = buffer;
    rx_length = length;
    rx_index = 0;
    rx_callback = callback;

    USART0->CMD = USART_CMD_CLEARRX;

    NVIC_ClearPendingIRQ(USART0_RX_IRQn);
    NVIC_EnableIRQ(USART0_RX_IRQn);
    USART0->IEN |= USART_IEN_RXDATAV;
}

static
void start_async_tx(uint8_t *buffer, uint32_t length, void (*callback)(void)) {
    tx_buffer = buffer;
    tx_length = length;
    tx_index = 0;
    tx_callback = callback;

    USART0->CMD = USART_CMD_CLEARTX;

    NVIC_ClearPendingIRQ(USART0_TX_IRQn);
    NVIC_EnableIRQ(USART0_TX_IRQn);
    USART0->IEN |= USART_IEN_TXBL;
}

static
void start_async_transfer(void);

static
void complete(uint32_t flag) {
    async_flags |= flag;
    if (async_flags == (rx_complete_flag | tx_complete_flag)) {
        if (++async_index < async_count) {
            start_async_transfer();
        } else {
            async_flags = complete_flag;
            if (async_callback != 0) {
                (*async_callback)();
            }
        }
    }
}

static
void rx_complete(void) {
    complete(rx_complete_flag);
}

static
void tx_complete(void) {
    complete(tx_complete_flag);
}

static
void start_async_transfer(void) {
    async_flags = 0;

    fd_spi_transfer *transfer = &async_transfers[async_index];
    if (transfer->op & fd_spi_op_read) {
        start_async_rx(transfer->rx_buffer, transfer->length, rx_complete);
    } else {
        async_flags |= rx_complete_flag;
    }
    if (transfer->op & fd_spi_op_write) {
        start_async_tx(transfer->tx_buffer, transfer->length, tx_complete);
    } else {
        async_flags |= tx_complete_flag;
    }
}

void fd_spi0_io(fd_spi_transfer *transfers, uint32_t count, void (*callback)(void)) {
    async_transfers = transfers;
    async_count = count;
    async_callback = callback;
    async_index = 0;

    start_async_transfer();
}

void fd_spi0_wait(void) {
    while (async_flags != complete_flag);
}

#define SPI_READ 0x80

uint8_t fd_spi0_read(uint8_t address) {
    uint8_t addr = SPI_READ | address;
    uint8_t result;
    fd_spi_transfer transfers[2] = {
        {
            .op = fd_spi_op_write,
            .length = 1,
            .tx_buffer = &addr,
            .rx_buffer = 0
        },
        {
            .op = fd_spi_op_read,
            .length = 1,
            .tx_buffer = 0,
            .rx_buffer = &result
        }
    };
    fd_spi0_io(transfers, 2, 0);
    fd_spi0_wait();
}

uint8_t fd_spi0_sync_io(uint8_t txdata) {
    USART0->TXDATA = txdata;
    while ((USART0->STATUS & USART_STATUS_TXC) == 0);
    uint8_t rxdata = USART0->RXDATA;
    return rxdata;
}

uint8_t fd_spi0_sync_read(uint8_t address) {
    fd_spi0_sync_io(SPI_READ | address);
    return fd_spi0_sync_io(0);
}