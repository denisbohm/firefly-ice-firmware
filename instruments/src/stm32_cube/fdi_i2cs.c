#include "fdi_i2cs.h"

#include "fdi_stm32.h"

#include "fd_log.h"

/* I2C TIMING Register define when I2C clock source is SYSCLK */
/* I2C TIMING is calculated in case of the I2C Clock source is the SYSCLK = 80 MHz */
/* This example use TIMING to 0x00D00E28 to reach 1 MHz speed (Rise time = 120ns, Fall time = 25ns) */
#define I2C_TIMING 0x00D00E28

#define I2C_SCL_PIN (1 << 8)
#define I2C_SDA_PIN (1 << 9)

typedef struct {
    fdi_i2cs_configuration_t configuration;
    I2C_HandleTypeDef i2c_handle;
    DMA_HandleTypeDef hdma_tx;
    DMA_HandleTypeDef hdma_rx;
    uint8_t tx_buffer[64];
    uint8_t rx_buffer[64];
} fdi_i2cs_t;

static fdi_i2cs_t fdi_i2cs;

void fdi_i2cs_initialize(fdi_i2cs_configuration_t configuration) {
    fdi_i2cs.configuration = configuration;

    RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct;
    RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    RCC_PeriphCLKInitStruct.I2c1ClockSelection = RCC_I2C1CLKSOURCE_SYSCLK;
    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    // I2C TX GPIO pin configuration
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = I2C_SCL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // I2C RX GPIO pin configuration
    GPIO_InitStruct.Pin = I2C_SDA_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    fdi_i2cs.hdma_tx.Instance = DMA1_Channel6;
    fdi_i2cs.hdma_tx.Init.Request = DMA_REQUEST_3;
    fdi_i2cs.hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    fdi_i2cs.hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    fdi_i2cs.hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
    fdi_i2cs.hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    fdi_i2cs.hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    fdi_i2cs.hdma_tx.Init.Mode = DMA_NORMAL;
    fdi_i2cs.hdma_tx.Init.Priority = DMA_PRIORITY_LOW;

    HAL_StatusTypeDef status = HAL_DMA_Init(&fdi_i2cs.hdma_tx);
    fd_log_assert(status == HAL_OK);

    __HAL_LINKDMA(&fdi_i2cs.i2c_handle, hdmatx, fdi_i2cs.hdma_tx);

    fdi_i2cs.hdma_rx.Instance = DMA1_Channel7;
    fdi_i2cs.hdma_rx.Init.Request = DMA_REQUEST_3;
    fdi_i2cs.hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    fdi_i2cs.hdma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    fdi_i2cs.hdma_rx.Init.MemInc = DMA_MINC_ENABLE;
    fdi_i2cs.hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    fdi_i2cs.hdma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    fdi_i2cs.hdma_rx.Init.Mode = DMA_NORMAL;
    fdi_i2cs.hdma_rx.Init.Priority = DMA_PRIORITY_HIGH;

    status = HAL_DMA_Init(&fdi_i2cs.hdma_rx);
    fd_log_assert(status == HAL_OK);

    __HAL_LINKDMA(&fdi_i2cs.i2c_handle, hdmarx, fdi_i2cs.hdma_rx);

    HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

    HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 2);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);

    fdi_i2cs.i2c_handle.Instance = I2C1;
    fdi_i2cs.i2c_handle.Init = (I2C_InitTypeDef) {
        .Timing           = I2C_TIMING,
        .AddressingMode   = I2C_ADDRESSINGMODE_7BIT,
        .DualAddressMode  = I2C_DUALADDRESS_DISABLE,
        .GeneralCallMode  = I2C_GENERALCALL_DISABLE,
        .NoStretchMode    = I2C_NOSTRETCH_DISABLE,
        .OwnAddress1      = fdi_i2cs.configuration.address << 1,
        .OwnAddress2      = 0xFF,
    };
    status = HAL_I2C_Init(&fdi_i2cs.i2c_handle);
    fd_log_assert(status == HAL_OK);

    HAL_I2CEx_ConfigAnalogFilter(&fdi_i2cs.i2c_handle, I2C_ANALOGFILTER_ENABLE);

    status = HAL_I2C_Slave_Receive_DMA(&fdi_i2cs.i2c_handle, fdi_i2cs.rx_buffer, sizeof(fdi_i2cs.rx_buffer));
    fd_log_assert(status == HAL_OK);
}

bool fdi_i2cs_can_transmit(void) {
    return true;
}

void fdi_i2cs_transmit(uint8_t *data, size_t size) {
    HAL_StatusTypeDef status = HAL_I2C_Slave_Transmit_DMA(&fdi_i2cs.i2c_handle, fdi_i2cs.tx_buffer, sizeof(fdi_i2cs.tx_buffer));
    fd_log_assert(status == HAL_OK);
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *I2cHandle) {
    fdi_i2cs.configuration.tx_ready();
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *I2cHandle) {
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *I2cHandle) {
    fdi_i2cs.configuration.rx(fdi_i2cs.rx_buffer, sizeof(fdi_i2cs.rx_buffer));
}

void I2C1_EV_IRQHandler(void) {
    HAL_I2C_EV_IRQHandler(&fdi_i2cs.i2c_handle);
}

void I2C1_ER_IRQHandler(void) {
    HAL_I2C_ER_IRQHandler(&fdi_i2cs.i2c_handle);
}

void DMA1_Channel6_IRQHandler(void) {
    HAL_DMA_IRQHandler(fdi_i2cs.i2c_handle.hdmatx);
}

void DMA1_Channel7_IRQHandler(void) {
    HAL_DMA_IRQHandler(fdi_i2cs.i2c_handle.hdmarx);
}