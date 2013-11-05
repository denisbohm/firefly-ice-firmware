#include "fd_nrf8001.h"
#include "fd_nrf8001_dispatch.h"
#include "fd_nrf8001_test.h"
#include "fd_processor.h"

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"

void halt(void) {
    *(uint32_t *)0xE000EDF0 = 0xA05F0003; // DHCSR = DBGKEY | C_HALT | C_DEBUGEN;
}

void GPIO_PinModeSet(GPIO_Port_TypeDef port,
                     unsigned int pin,
                     GPIO_Mode_TypeDef mode,
                     unsigned int out)
{
  /* If disabling pin, do not modify DOUT in order to reduce chance for */
  /* glitch/spike (may not be sufficient precaution in all use cases) */
  if (mode != gpioModeDisabled)
  {
    if (out)
    {
      GPIO->P[port].DOUTSET = 1 << pin;
    }
    else
    {
      GPIO->P[port].DOUTCLR = 1 << pin;
    }
  }

  /* There are two registers controlling the pins for each port. The MODEL
   * register controls pins 0-7 and MODEH controls pins 8-15. */
  if (pin < 8)
  {
    GPIO->P[port].MODEL = (GPIO->P[port].MODEL & ~(0xF << (pin * 4))) |
                          (mode << (pin * 4));
  }
  else
  {
    GPIO->P[port].MODEH = (GPIO->P[port].MODEH & ~(0xF << ((pin - 8) * 4))) |
                          (mode << ((pin - 8) * 4));
  }

  if (mode == gpioModeDisabled)
  {
    if (out)
    {
      GPIO->P[port].DOUTSET = 1 << pin;
    }
    else
    {
      GPIO->P[port].DOUTCLR = 1 << pin;
    }
  }
}

void GPIO_IntConfig(GPIO_Port_TypeDef port,
                    unsigned int pin,
                    bool risingEdge,
                    bool fallingEdge,
                    bool enable)
{
}

void CMU_Sync(uint32_t mask)
{
  /* Avoid deadlock if modifying the same register twice when freeze mode is */
  /* activated. */
  if (CMU->FREEZE & CMU_FREEZE_REGFREEZE)
    return;

  /* Wait for any pending previous write operation to have been completed */
  /* in low frequency domain */
  while (CMU->SYNCBUSY & mask)
    ;
}

void CMU_ClockEnable(CMU_Clock_TypeDef clock, bool enable)
{
  volatile uint32_t *reg;
  uint32_t          bit;
  uint32_t          sync = 0;

  /* Identify enable register */
  switch ((clock >> CMU_EN_REG_POS) & CMU_EN_REG_MASK)
  {
  case CMU_HFPERCLKDIV_EN_REG:
    reg = &(CMU->HFPERCLKDIV);
    break;

  case CMU_HFPERCLKEN0_EN_REG:
    reg = &(CMU->HFPERCLKEN0);
    break;

  case CMU_HFCORECLKEN0_EN_REG:
    reg = &(CMU->HFCORECLKEN0);
    break;

  case CMU_LFACLKEN0_EN_REG:
    reg  = &(CMU->LFACLKEN0);
    sync = CMU_SYNCBUSY_LFACLKEN0;
    break;

  case CMU_LFBCLKEN0_EN_REG:
    reg  = &(CMU->LFBCLKEN0);
    sync = CMU_SYNCBUSY_LFBCLKEN0;
    break;

  case CMU_PCNT_EN_REG:
    reg = &(CMU->PCNTCTRL);
    break;

  default: /* Cannot enable/disable clock point */
    EFM_ASSERT(0);
    return;
  }

  /* Get bit position used to enable/disable */
  bit = (clock >> CMU_EN_BIT_POS) & CMU_EN_BIT_MASK;

  /* LF synchronization required? */
  if (sync)
  {
    CMU_Sync(sync);
  }

  /* Set/clear bit as requested */
  BITBAND_Peripheral(reg, bit, (unsigned int)enable);
}

void spi_initialize(void) {
    CMU_ClockEnable(cmuClock_USART1, true);

    USART1->CMD = USART_CMD_RXDIS | USART_CMD_TXDIS | USART_CMD_MASTERDIS | USART_CMD_RXBLOCKDIS | USART_CMD_TXTRIDIS | USART_CMD_CLEARTX | USART_CMD_CLEARRX;
    USART1->CTRL     = _USART_CTRL_RESETVALUE;
    USART1->FRAME    = _USART_FRAME_RESETVALUE;
    USART1->TRIGCTRL = _USART_TRIGCTRL_RESETVALUE;
    USART1->CLKDIV   = _USART_CLKDIV_RESETVALUE;
    USART1->IEN      = _USART_IEN_RESETVALUE;
    USART1->IFC      = _USART_IFC_MASK;
    USART1->ROUTE    = _USART_ROUTE_RESETVALUE;

    USART1->CTRL |= USART_CTRL_SYNC | USART_CTRL_CLKPOL_IDLELOW | USART_CTRL_CLKPHA_SAMPLELEADING;
    USART1->FRAME = USART_FRAME_DATABITS_EIGHT | USART_FRAME_STOPBITS_DEFAULT | USART_FRAME_PARITY_DEFAULT;

    USART1->CLKDIV = 1536; // 1000000 baud

    USART1->CMD = USART_CMD_MASTEREN;

    USART1->ROUTE = USART_ROUTE_TXPEN | USART_ROUTE_RXPEN | USART_ROUTE_CLKPEN | USART_ROUTE_LOCATION_LOC1;

    USART1->CMD = USART_CMD_RXEN | USART_CMD_TXEN;
}

uint8_t spi_io(uint8_t data) {
    USART1->TXDATA = data;
    while (!(USART1->STATUS & USART_STATUS_TXC));
    return USART1->RXDATA;
}

void fd_bluetooth_reset(void) {
    GPIO_PinOutClear(NRF_RESETN_PORT_PIN);
    fd_delay_ms(100);
    GPIO_PinOutSet(NRF_RESETN_PORT_PIN);
    fd_delay_ms(100); // wait for nRF8001 to come out of reset (62ms)
}

uint8_t xfers[2048];
uint32_t xfers_index;

bool fd_bluetooth_spi_transfer(void) {
    if (GPIO_PinInGet(NRF_RDYN_PORT_PIN)) {
        return false;
    }

    for (uint32_t i = 0; i < fd_nrf8001_spi_tx_length; ++i) {
        xfers[xfers_index++] = fd_nrf8001_spi_tx_buffer[i];
        if (xfers_index >= 2048) {
            xfers_index = 0;
        }
    }

    uint8_t rx_buffer[FD_NRF8001_SPI_RX_BUFFER_SIZE];
    GPIO_PinOutClear(NRF_REQN_PORT_PIN);
    uint32_t tx_index = 0;
    uint8_t status_byte = spi_io(fd_nrf8001_spi_tx_buffer[tx_index++]);
    rx_buffer[0] = spi_io(fd_nrf8001_spi_tx_buffer[tx_index++]);
    uint32_t tx_rx_count;
    if (fd_nrf8001_spi_tx_length == 0) {
        tx_rx_count = rx_buffer[0];
    } else {
        tx_rx_count = (rx_buffer[0] > (fd_nrf8001_spi_tx_length - 1)) ? rx_buffer[0] : (fd_nrf8001_spi_tx_length - 1);
    }
    uint32_t i;
    for (i = 0; i < tx_rx_count; i++) {
        rx_buffer[i + 1] = spi_io(fd_nrf8001_spi_tx_buffer[tx_index++]);
    }
    GPIO_PinOutSet(NRF_REQN_PORT_PIN);

    fd_nrf8001_spi_tx_clear();

    uint32_t length = rx_buffer[0];

    for (uint32_t i = 0; i <= length; ++i) {
        xfers[xfers_index++] = rx_buffer[i];
        if (xfers_index >= 2048) {
            xfers_index = 0;
        }
    }

    if (length > 0) {
        fd_nrf8001_dispatch(&rx_buffer[1], length);
    }
    return true;
}

void fd_nrf8001_spi_transfer(void) {
    GPIO_PinOutClear(NRF_REQN_PORT_PIN);
    while (GPIO_PinInGet(NRF_RDYN_PORT_PIN));

    fd_bluetooth_spi_transfer();
}

void main(void) {
    xfers_index = 0;

    fd_processor_initialize();
    spi_initialize();
    fd_bluetooth_reset();
    uint8_t data[10];
    uint32_t result __attribute__ ((unused)) = fd_nrf8001_test_broadcast(data, sizeof(data));
    halt();
}