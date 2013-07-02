#include "fd_log.h"
#include "fd_processor.h"
#include "fd_spi0.h"

#include <em_cmu.h>
#include <em_gpio.h>
#include <em_usart.h>

#include <string.h>

#define BUFFER_SIZE 260

static void (*transfer_callback)(void);
static uint32_t transfer_length;
static uint32_t transfer_flags;
#define rx_complete_flag 0x01
#define tx_complete_flag 0x02
#define complete_flag 0x04

static uint8_t rx_buffer[BUFFER_SIZE];
static uint32_t rx_length;
static uint32_t rx_index;

static uint8_t tx_buffer[BUFFER_SIZE];
static uint32_t tx_length;
static uint32_t tx_index;

void fd_spi0_initialize(void) {
    tx_length = 0;
    rx_length = 0;

    CMU_ClockEnable(cmuClock_USART0, true);

    USART_InitSync_TypeDef spi_setup = USART_INITSYNC_DEFAULT;
    spi_setup.msbf = false;
    spi_setup.clockMode = usartClockMode0;
    spi_setup.baudrate = 3000000;
    USART_InitSync(USART0, &spi_setup);

    USART0->ROUTE = USART_ROUTE_TXPEN | USART_ROUTE_RXPEN | USART_ROUTE_CLKPEN | US0_LOCATION;

    USART_Enable(USART0, usartEnable);
}

void fd_spi0_sleep(void) {
    USART_Enable(USART0, usartDisable);
    CMU_ClockEnable(cmuClock_USART0, false);
}

void fd_spi0_wake(void) {
    CMU_ClockEnable(cmuClock_USART0, true);
    USART_Enable(USART0, usartEnable);
}

void fd_spi0_power_on(void) {
    // power up memory section
    GPIO_PinOutSet(US0_PWR_PORT_PIN);
    fd_delay_ms(10); // 10ms delay for memory to power up
}

void fd_spi0_power_off(void) {
    CMU_ClockEnable(cmuClock_USART0, false);

    USART0->ROUTE = 0;

    GPIO_PinModeSet(US0_CLK_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(US0_MISO_PORT_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(US0_MOSI_PORT_PIN, gpioModePushPull, 0);

    GPIO_PinModeSet(MEM_CSN_PORT_PIN, gpioModePushPull, 0);

    GPIO_PinOutClear(US0_PWR_PORT_PIN);
}

void fd_spi0_tx_clear(void) {
    tx_length = 0;
}

void fd_spi0_tx_queue(uint8_t *buffer, uint32_t length) {
    if ((tx_length + length) > BUFFER_SIZE) {
        fd_log_assert_fail("");
        return;
    }

    memcpy(&tx_buffer[tx_length], buffer, length);
    tx_length += length;
}

static
void complete(uint32_t flag) {
    transfer_flags |= flag;
    if (transfer_flags == (rx_complete_flag | tx_complete_flag)) {
        transfer_flags = complete_flag;
        if (transfer_callback != 0) {
            (*transfer_callback)();
        }
    }
}

void USART0_RX_IRQHandler(void) {
    if (USART0->STATUS & USART_STATUS_RXDATAV) {
        uint8_t rxdata = USART0->RXDATA;
        if (rx_index < rx_length) {
            // 2nd byte is the number of rx bytes to follow
            if (rx_index == 1) {
                rx_length = rxdata + 2 /* dummy byte and length byte */;
                if (rx_length > transfer_length) {
                    transfer_length = rx_length;
                }
            }
            rx_buffer[rx_index++] = rxdata;
        }
        if (rx_index >= rx_length) {
            NVIC_DisableIRQ(USART0_RX_IRQn);
            USART0->IEN &= ~USART_IEN_RXDATAV;
            complete(rx_complete_flag);
        }
    }
}

void USART0_TX_IRQHandler(void) {
    if (USART0->STATUS & USART_STATUS_TXBL) {
        if (tx_index < tx_length) {
            USART0->TXDATA = tx_buffer[tx_index++];
        } else
        if (tx_index < transfer_length) {
            USART0->TXDATA = 0;
            tx_index++;
        } else {
            NVIC_DisableIRQ(USART0_TX_IRQn);
            USART0->IEN &= ~USART_IEN_TXBL;
            complete(tx_complete_flag);
        }
    }
}

void fd_spi0_start_transfer(void (*callback)(void)) {
    transfer_callback = callback;

    transfer_length = tx_length;
    rx_length = 2; // don't know how much to receive until 2nd byte with the length is read
    if (rx_length > transfer_length) {
        transfer_length = rx_length;
    }

    rx_index = 0;
    USART0->CMD = USART_CMD_CLEARRX;
    NVIC_ClearPendingIRQ(USART0_RX_IRQn);
    NVIC_EnableIRQ(USART0_RX_IRQn);
    USART0->IEN |= USART_IEN_RXDATAV;

    tx_index = 0;
    if (tx_length > 0) {
        USART0->CMD = USART_CMD_CLEARTX;
        NVIC_ClearPendingIRQ(USART0_TX_IRQn);
        NVIC_EnableIRQ(USART0_TX_IRQn);
        USART0->IEN |= USART_IEN_TXBL;
    } else {
        transfer_flags |= tx_complete_flag;
    }
}

void fd_spi0_wait(void) {
    while (transfer_flags != complete_flag);
}

uint8_t fd_spi0_sync_io(uint8_t txdata) {
    USART0->TXDATA = txdata;
    while ((USART0->STATUS & USART_STATUS_TXC) == 0);
    uint8_t rxdata = USART0->RXDATA;
    return rxdata;
}

void fd_spi0_sync_transfer(void) {
    transfer_length = tx_length;
    rx_length = 2; // don't know how much to receive until 2nd byte with the length is read
    if (rx_length > transfer_length) {
        transfer_length = rx_length;
    }

    rx_index = 0;
    tx_index = 0;

    uint8_t index = 0;
    while (index++ < transfer_length) {
        uint8_t txdata = 0;
        if (tx_index < tx_length) {
            txdata = tx_buffer[tx_index++];
        }
        uint8_t rxdata = fd_spi0_sync_io(txdata);
        if (rx_index < rx_length) {
            rx_buffer[rx_index++] = rxdata;
        }
        if (rx_index == 2) {
            rx_length = rxdata + 2;
            if (rx_length > transfer_length) {
                transfer_length = rx_length;
            }
        }
    }
}

void fd_spi0_get_rx(uint8_t **pbuffer, uint32_t *plength) {
    *pbuffer = rx_buffer;
    *plength = rx_length;
}

void fd_spi0_rx_clear(void) {
    rx_length = 0;
}