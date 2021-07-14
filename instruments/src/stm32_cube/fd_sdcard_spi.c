#include "fd_spim.h"

#include "fd_log.h"

#include "fdi_stm32.h"

typedef struct {
    SPI_HandleTypeDef spi;
} fd_sdcard_spi_t;

fd_sdcard_spi_t fd_sdcard_spi;

void fd_sdcard_spi_initialize(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio_init = {
        .Pin       = 4,
        .Mode      = GPIO_MODE_AF_PP,
        .Pull      = GPIO_PULLDOWN,
        .Speed     = GPIO_SPEED_FREQ_HIGH,
        .Alternate = 0,
    };
    // NSS
    HAL_GPIO_Init(GPIOA, &gpio_init);

    // SCK
    gpio_init.Pin       = 5,
    gpio_init.Alternate = GPIO_AF5_SPI1,
    HAL_GPIO_Init(GPIOA, &gpio_init);

    // MISO
    gpio_init.Pin = GPIO_AF5_SPI1;
    gpio_init.Alternate = 6;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    // MOSI
    gpio_init.Pin = GPIO_AF5_SPI1;
    gpio_init.Alternate = 7;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    __HAL_RCC_SPI1_CLK_ENABLE();
    fd_sdcard_spi.spi = (SPI_HandleTypeDef) {
        .Instance = SPI1,
        .Init = {
            .BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256,
            .Direction         = SPI_DIRECTION_2LINES,
            .CLKPhase          = SPI_PHASE_1EDGE,
            .CLKPolarity       = SPI_POLARITY_LOW,
            .DataSize          = SPI_DATASIZE_8BIT,
            .FirstBit          = SPI_FIRSTBIT_MSB,
            .TIMode            = SPI_TIMODE_DISABLE,
            .CRCCalculation    = SPI_CRCCALCULATION_DISABLE,
            .CRCPolynomial     = 7,
            .CRCLength         = SPI_CRC_LENGTH_8BIT,
            .NSS               = SPI_NSS_SOFT,
            .NSSPMode          = SPI_NSS_PULSE_DISABLE,
            .Mode              = SPI_MODE_MASTER,
        },
    };
    HAL_StatusTypeDef status = HAL_SPI_Init(&fd_sdcard_spi.spi);
    fd_log_assert(status == HAL_OK);
}

void fd_sdcard_spi_set_chip_select(bool value) {
    fd_gpio_set((fd_gpio_t) { .port = 0, .pin = 4 }, value);
}

static
void fd_sdcard_spi_set_frequency(uint32_t prescaler) {
    if (fd_sdcard_spi.spi.Init.BaudRatePrescaler == prescaler) {
        return;
    }

    HAL_StatusTypeDef status = HAL_SPI_DeInit(&fd_sdcard_spi.spi);
    fd_log_assert(status == HAL_OK);
    fd_sdcard_spi.spi.Init.BaudRatePrescaler = prescaler;
    status = HAL_SPI_Init(&fd_sdcard_spi.spi);
    fd_log_assert(status == HAL_OK);
}

// frequency <= 400 kHz
void fd_sdcard_spi_set_frequency_slow(void) {
    fd_sdcard_spi_set_frequency(SPI_BAUDRATEPRESCALER_256);
}

// frequency <= 25 MHz
void fd_sdcard_spi_set_frequency_fast(void) {
    fd_sdcard_spi_set_frequency(SPI_BAUDRATEPRESCALER_4);
}

void fd_sdcard_spi_transceive(const uint8_t *tx, size_t tx_count, uint8_t *rx, size_t rx_count) {
    if (tx_count == rx_count) {
        uint8_t *tx_data = (uint8_t *)tx;
        uint8_t *rx_data = rx;
        uint16_t size = (uint16_t)rx_count;
        HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&fd_sdcard_spi.spi, tx_data, rx_data, size, 100);
        fd_log_assert(status == HAL_OK);
    } else
    if (rx_count == 0) {
        uint8_t *data = (uint8_t *)tx;
        uint16_t size = (uint16_t)tx_count;
        HAL_StatusTypeDef status = HAL_SPI_Transmit(&fd_sdcard_spi.spi, data, size, 100);
        fd_log_assert(status == HAL_OK);
    } else
    if (tx_count == 0) {
        uint8_t *data = rx;
        uint16_t size = (uint16_t)rx_count;
        HAL_StatusTypeDef status = HAL_SPI_Receive(&fd_sdcard_spi.spi, data, size, 100);
        fd_log_assert(status == HAL_OK);
    } else
    if (tx_count < rx_count) {
        uint8_t *tx_data = (uint8_t *)tx;
        uint8_t *rx_data = rx;
        uint16_t size = (uint16_t)rx_count;
        HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&fd_sdcard_spi.spi, tx_data, rx_data, size, 100);
        fd_log_assert(status == HAL_OK);
        uint8_t *r_data = &rx[tx_count];
        uint16_t r_size = (uint16_t)(rx_count - tx_count);
        status = HAL_SPI_Receive(&fd_sdcard_spi.spi, r_data, r_size, 100);
        fd_log_assert(status == HAL_OK);
    } else {
        uint8_t *tx_data = (uint8_t *)tx;
        uint8_t *rx_data = rx;
        uint16_t size = (uint16_t)rx_count;
        HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&fd_sdcard_spi.spi, tx_data, rx_data, size, 100);
        fd_log_assert(status == HAL_OK);
        uint8_t *t_data = (uint8_t *)&tx[rx_count];
        uint16_t t_size = (uint16_t)(tx_count - rx_count);
        status = HAL_SPI_Transmit(&fd_sdcard_spi.spi, t_data, t_size, 100);
        fd_log_assert(status == HAL_OK);
    }
}