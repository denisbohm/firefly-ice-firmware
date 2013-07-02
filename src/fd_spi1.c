#include "fd_processor.h"
#include "fd_spi1.h"

#include <em_cmu.h>
#include <em_usart.h>

static GPIO_Port_TypeDef async_csn_port;
static unsigned int async_csn_pin;
static fd_spi_transfer *async_transfers;
static uint32_t async_count;
static void (*async_callback)(void);
static uint32_t async_index;
static uint32_t async_flags;
#define started_flag 0x01
#define rx_complete_flag 0x02
#define tx_complete_flag 0x04
#define complete_flag 0x08

static uint8_t *rx_buffer;
static uint32_t rx_length;
static uint32_t rx_index;
static void (*rx_callback)(void);

static uint8_t *tx_buffer;
static uint32_t tx_length;
static uint32_t tx_index;
static void (*tx_callback)(void);

void fd_spi1_initialize(void) {
    CMU_ClockEnable(cmuClock_USART1, true);

    USART_InitSync_TypeDef spi_setup = USART_INITSYNC_DEFAULT;
    spi_setup.msbf = true;
    spi_setup.clockMode = usartClockMode3;
    spi_setup.baudrate = 10000000;
    USART_InitSync(USART1, &spi_setup);

    USART1->ROUTE = USART_ROUTE_TXPEN | USART_ROUTE_RXPEN | USART_ROUTE_CLKPEN | US1_LOCATION;

    USART_Enable(USART1, usartEnable);
}

void fd_spi1_sleep(void) {
    USART_Enable(USART1, usartDisable);
    CMU_ClockEnable(cmuClock_USART1, false);
}

void fd_spi1_wake(void) {
    CMU_ClockEnable(cmuClock_USART1, true);
    USART_Enable(USART1, usartEnable);
}

void USART1_RX_IRQHandler(void) {
    if (USART1->STATUS & USART_STATUS_RXDATAV) {
        uint8_t rxdata = USART1->RXDATA;
        if (rx_index < rx_length) {
            rx_buffer[rx_index++] = rxdata;
        } else {
            NVIC_DisableIRQ(USART1_RX_IRQn);
            USART1->IEN &= ~USART_IEN_RXDATAV;
            if (rx_callback != 0) {
                (*rx_callback)();
            }
        }
    }
}

void USART1_TX_IRQHandler(void) {
    if (USART1->STATUS & USART_STATUS_TXBL) {
        if (tx_index < tx_length) {
            USART1->TXDATA = tx_buffer[tx_index++];
        } else {
            NVIC_DisableIRQ(USART1_TX_IRQn);
            USART1->IEN &= ~USART_IEN_TXBL;
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

    USART1->CMD = USART_CMD_CLEARRX;

    NVIC_ClearPendingIRQ(USART1_RX_IRQn);
    NVIC_EnableIRQ(USART1_RX_IRQn);
    USART1->IEN |= USART_IEN_RXDATAV;
}

static
void start_async_tx(uint8_t *buffer, uint32_t length, void (*callback)(void)) {
    tx_buffer = buffer;
    tx_length = length;
    tx_index = 0;
    tx_callback = callback;

    USART1->CMD = USART_CMD_CLEARTX;

    NVIC_ClearPendingIRQ(USART1_TX_IRQn);
    NVIC_EnableIRQ(USART1_TX_IRQn);
    USART1->IEN |= USART_IEN_TXBL;
}

static
void start_async_transfer(void);

static
void complete(uint32_t flag) {
    async_flags |= flag;
    if (async_flags == (started_flag | rx_complete_flag | tx_complete_flag)) {
        if (++async_index < async_count) {
            start_async_transfer();
        } else {
            GPIO_PinOutSet(async_csn_port, async_csn_pin);
            async_flags |= complete_flag;
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
    async_flags = started_flag;

    GPIO_PinOutClear(async_csn_port, async_csn_pin);

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

void fd_spi1_io(GPIO_Port_TypeDef csn_port, unsigned int csn_pin, fd_spi_transfer *transfers, uint32_t count, void (*callback)(void)) {
    async_csn_port = csn_port;
    async_csn_pin = csn_pin;
    async_transfers = transfers;
    async_count = count;
    async_callback = callback;
    async_index = 0;

    start_async_transfer();
}

void fd_spi1_wait(void) {
    if (async_flags & started_flag) {
        while (!(async_flags & complete_flag));
    }
}

#define SPI_READ 0x80
#define SPI_ADDRESS_INCREMENT 0x40

uint8_t fd_spi1_read(GPIO_Port_TypeDef csn_port, unsigned int csn_pin, uint8_t address) {
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
    fd_spi1_io(csn_port, csn_pin, transfers, 2, 0);
    fd_spi1_wait();
    return result;
}

uint8_t fd_spi1_sync_io(uint8_t txdata) {
    USART1->TXDATA = txdata;
    while ((USART1->STATUS & USART_STATUS_TXC) == 0);
    uint8_t rxdata = USART1->RXDATA;
    return rxdata;
}

uint8_t fd_spi1_sync_read(uint8_t address) {
    fd_spi1_sync_io(SPI_READ | address);
    return fd_spi1_sync_io(0);
}

void fd_spi1_sync_read_bytes(uint8_t address, uint8_t *bytes, uint32_t length) {
    fd_spi1_sync_io(SPI_READ | SPI_ADDRESS_INCREMENT | address);
    for (uint32_t i = 0; i < length; ++i) {
        bytes[i] = fd_spi1_sync_io(0);
    }
}

void fd_spi1_sync_write(uint8_t address, uint8_t value) {
    fd_spi1_sync_io(address);
    fd_spi1_sync_io(value);
}