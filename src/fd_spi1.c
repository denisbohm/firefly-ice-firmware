#include "fd_spi1.h"

#include <em_usart.h>

#include <string.h>

#define BUFFER_SIZE 32

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

void fd_spi1_initialize(void) {
    tx_length = 0;
    rx_length = 0;

    USART_InitSync_TypeDef spi_setup = USART_INITSYNC_DEFAULT;
    USART_InitSync(USART1, &spi_setup);
}

void fd_spi1_tx_clear(void) {
    tx_length = 0;
}

void fd_spi1_tx_queue(uint8_t *buffer, uint32_t length) {
    if ((tx_length + length) > BUFFER_SIZE) {
        // log diagnostic
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

void USART1_RX_IRQHandler(void) {
    if (USART1->STATUS & USART_STATUS_RXDATAV) {
        uint8_t rxdata = USART1->RXDATA;
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
            NVIC_DisableIRQ(USART1_RX_IRQn);
            USART1->IEN &= ~USART_IEN_RXDATAV;
            complete(rx_complete_flag);
        }
    }
}

void USART1_TX_IRQHandler(void) {
    if (USART1->STATUS & USART_STATUS_TXBL) {
        if (tx_index < tx_length) {
            USART1->TXDATA = tx_buffer[tx_index++];
        } else
        if (tx_index < transfer_length) {
            USART1->TXDATA = 0;
            tx_index++;
        } else {
            NVIC_DisableIRQ(USART1_TX_IRQn);
            USART1->IEN &= ~USART_IEN_TXBL;
            complete(tx_complete_flag);
        }
    }
}

void fd_spi1_start_transfer(void (*callback)(void)) {
    transfer_callback = callback;

    transfer_length = tx_length;
    rx_length = 2; // don't know how much to receive until 2nd byte with the length is read
    if (rx_length > transfer_length) {
        transfer_length = rx_length;
    }

    rx_index = 0;
    USART1->CMD = USART_CMD_CLEARRX;
    NVIC_ClearPendingIRQ(USART1_RX_IRQn);
    NVIC_EnableIRQ(USART1_RX_IRQn);
    USART1->IEN |= USART_IEN_RXDATAV;

    tx_index = 0;
    if (tx_length > 0) {
        USART1->CMD = USART_CMD_CLEARTX;
        NVIC_ClearPendingIRQ(USART1_TX_IRQn);
        NVIC_EnableIRQ(USART1_TX_IRQn);
        USART1->IEN |= USART_IEN_TXBL;
    } else {
        transfer_flags |= tx_complete_flag;
    }
}

void fd_spi1_wait(void) {
    while (transfer_flags != complete_flag);
}

void fd_spi1_get_rx(uint8_t **pbuffer, uint32_t *plength) {
    *pbuffer = rx_buffer;
    *plength = rx_length;
}

void fd_spi1_rx_clear(void) {
    rx_length = 0;
}