#include "fdi_i2cs.h"

#include "fdi_stm32.h"

#include "fd_log.h"

#include <string.h>

//#define FDI_I2CS_DMA

/* I2C TIMING Register define when I2C clock source is SYSCLK */
/* I2C TIMING is calculated in case of the I2C Clock source is the SYSCLK = 80 MHz */
/* This example use TIMING to 0x00D00E28 to reach 1 MHz speed (Rise time = 120ns, Fall time = 25ns) */
#define I2C_TIMING 0x00D00E28

#define I2C_SCL_PIN (1 << 8)
#define I2C_SDA_PIN (1 << 9)

typedef struct {
    fdi_i2cs_configuration_t configuration;
    I2C_HandleTypeDef i2c_handle;
#ifdef FDI_I2CS_DMA
    DMA_HandleTypeDef hdma_tx;
    DMA_HandleTypeDef hdma_rx;
#endif
    bool has_pending_tx;
    bool send_pending_tx;
    bool tx_underflow;
    bool tx_overflow;
    bool rx_underflow;
    bool rx_overflow;
    uint32_t tx_index;
    uint32_t rx_index;
    uint8_t tx_buffer[64];
    uint8_t rx_buffer[64];
} fdi_i2cs_t;

static fdi_i2cs_t fdi_i2cs;

void fdi_i2cs_initialize(fdi_i2cs_configuration_t configuration) {
    fdi_i2cs.configuration = configuration;

    __HAL_RCC_GPIOB_CLK_ENABLE();

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

#ifdef FDI_I2CS_DMA
    __HAL_RCC_DMA1_CLK_ENABLE();

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

    DMA1_Channel6->CCR = DMA_CCR_MINC | DMA_CCR_DIR;
    DMA1_CSELR->CSELR &= ~DMA_CSELR_C6S;
    DMA1_CSELR->CSELR |= 3 << DMA_CSELR_C6S_Pos;

    HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

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

    DMA1_Channel7->CCR = DMA_CCR_PL_1 | DMA_CCR_MINC;
    DMA1_CSELR->CSELR &= ~DMA_CSELR_C7S;
    DMA1_CSELR->CSELR |= 3 << DMA_CSELR_C7S_Pos;

    HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);
#endif

    __HAL_RCC_I2C1_CLK_ENABLE();

    RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct;
    RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    RCC_PeriphCLKInitStruct.I2c1ClockSelection = RCC_I2C1CLKSOURCE_SYSCLK;
    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);

    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);

    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 2);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);

#ifdef FDI_I2CS_HAL
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
#else
    I2C1->CR1 = 0x00000000;
    //
    I2C1->CR1 = I2C_CR1_ADDRIE | I2C_CR1_RXIE | I2C_CR1_STOPIE | I2C_CR1_NACKIE | I2C_CR1_ERRIE;
    I2C1->CR2 = 0x00000000;
    I2C1->OAR1 = I2C_OAR1_OA1EN | fdi_i2cs.configuration.address << 1;
    I2C1->OAR2 = 0x00000000;
    I2C1->TIMINGR = I2C_TIMING;
    //
    I2C1->CR1 |= I2C_CR1_PE;
#endif
}

bool fdi_i2cs_transmit(uint8_t *data, size_t size) {
    if (fdi_i2cs.has_pending_tx) {
        return false;
    }

    fd_log_assert(size <= sizeof(fdi_i2cs.tx_buffer));

    memcpy(fdi_i2cs.tx_buffer, data, size);
    memset(&fdi_i2cs.tx_buffer[size], 0, sizeof(fdi_i2cs.tx_buffer) - size);
#if 0
    for (int i = 0; i < 64; ++i) {
        fdi_i2cs.tx_buffer[i] = i;
    }
#endif
    fdi_i2cs.has_pending_tx = true;
    return true;
}

#ifdef FDI_I2CS_HAL

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *I2cHandle) {
    memset(fdi_i2cs.tx_buffer, 0, sizeof(fdi_i2cs.tx_buffer));
    fdi_i2cs.has_pending_tx = false;
    fdi_i2cs.configuration.tx_ready();
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *I2cHandle) {
}

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *I2cHandle, uint8_t TransferDirection, uint16_t AddrMatchCode) {
    if (TransferDirection == 0) {
        // write
        HAL_StatusTypeDef status = HAL_I2C_Slave_Receive_DMA(&fdi_i2cs.i2c_handle, fdi_i2cs.rx_buffer, sizeof(fdi_i2cs.rx_buffer));
        fd_log_assert(status == HAL_OK);
    } else {
        // read
        HAL_StatusTypeDef status = HAL_I2C_Slave_Transmit_DMA(&fdi_i2cs.i2c_handle, fdi_i2cs.tx_buffer, sizeof(fdi_i2cs.tx_buffer));
        fd_log_assert(status == HAL_OK);
    }
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *I2cHandle) {
    fdi_i2cs.configuration.rx(fdi_i2cs.rx_buffer, sizeof(fdi_i2cs.rx_buffer));
}

#endif

void I2C1_EV_IRQHandler(void) {
    uint32_t isr = I2C1->ISR;

    if (isr & I2C_ISR_ADDR) {
        if (isr & I2C_ISR_DIR) {
            // read

#ifdef FDI_I2CS_DMA
            DMA1_Channel7->CCR &= ~DMA_CCR_EN;
            //
            DMA1->IFCR = DMA_ISR_GIF7;
            DMA1_Channel7->CPAR = (uint32_t)&I2C1->TXDR;
            DMA1_Channel7->CMAR = (uint32_t)fdi_i2cs.tx_buffer;
            DMA1_Channel7->CNDTR = sizeof(fdi_i2cs.tx_buffer);
            DMA1_Channel7->CCR |= DMA_IT_TC | DMA_IT_TE;
            //
            DMA1_Channel7->CCR |= DMA_CCR_EN;

            I2C1->CR1 |= I2C_CR1_TXDMAEN;
#endif
            I2C1->ISR |= I2C_ISR_TXE;
            I2C1->CR1 |= I2C_CR1_TXIE;

            fdi_i2cs.send_pending_tx = fdi_i2cs.has_pending_tx;
        } else {
            // write

#ifdef FDI_I2CS_DMA
            DMA1_Channel6->CCR &= ~DMA_CCR_EN;
            //
            DMA1->IFCR = DMA_ISR_GIF6;
            DMA1_Channel6->CPAR = (uint32_t)&I2C1->RXDR;
            DMA1_Channel6->CMAR = (uint32_t)fdi_i2cs.rx_buffer;
            DMA1_Channel6->CNDTR = sizeof(fdi_i2cs.rx_buffer);
            DMA1_Channel6->CCR |= DMA_IT_TC | DMA_IT_TE;
            //
            DMA1_Channel6->CCR |= DMA_CCR_EN;

            I2C1->CR1 |= I2C_CR1_RXDMAEN;
#endif
        }

        fdi_i2cs.tx_index = 0;
        fdi_i2cs.rx_index = 0;

        I2C1->CR2 &= ~I2C_CR2_NACK;
        I2C1->ICR |= I2C_ICR_ADDRCF;
    }

    if (isr & I2C_ISR_RXNE) {
        if (fdi_i2cs.rx_index < sizeof(fdi_i2cs.rx_buffer)) {
            fdi_i2cs.rx_buffer[fdi_i2cs.rx_index++] = I2C1->RXDR;
        } else {
            uint8_t rxdr = I2C1->RXDR;
            fdi_i2cs.rx_overflow = true;
        }
    }

    if ((isr & I2C_ISR_DIR) && (isr & I2C_ISR_TXIS)) {
        if (fdi_i2cs.send_pending_tx && (fdi_i2cs.tx_index < sizeof(fdi_i2cs.tx_buffer))) {
            I2C1->TXDR = fdi_i2cs.tx_buffer[fdi_i2cs.tx_index++];
        } else {
            I2C1->TXDR = 0x00;
            fdi_i2cs.tx_overflow = true;
        }
    }

    if (isr & I2C_ISR_STOPF) {
        if (fdi_i2cs.rx_index == sizeof(fdi_i2cs.rx_buffer)) {
            fdi_i2cs.configuration.rx(fdi_i2cs.rx_buffer, sizeof(fdi_i2cs.rx_buffer));
        } else
        if (fdi_i2cs.rx_index > 0) {
            fdi_i2cs.rx_underflow = true;
        }

        fdi_i2cs.send_pending_tx = false;
        if (fdi_i2cs.tx_index == sizeof(fdi_i2cs.tx_buffer)) {
            fdi_i2cs.has_pending_tx = false;
            fdi_i2cs.configuration.tx_ready();
        } else
        if (fdi_i2cs.tx_index > 0) {
            fdi_i2cs.tx_underflow = true;
        }

        I2C1->CR1 &= ~I2C_CR1_TXIE;
        I2C1->ICR |= I2C_ISR_STOPF;
    }

    if (isr & I2C_ISR_NACKF) {
        I2C1->ICR = I2C_ICR_NACKCF;
    }

    if (isr & I2C_ISR_ARLO | I2C_ISR_BERR | I2C_ISR_OVR | I2C_ISR_TIMEOUT | I2C_ISR_PECERR | I2C_ISR_ALERT) {
    }

#ifdef FDI_I2CS_HAL
    HAL_I2C_EV_IRQHandler(&fdi_i2cs.i2c_handle);
#endif
}

void I2C1_ER_IRQHandler(void) {
#ifdef FDI_I2CS_HAL
    HAL_I2C_ER_IRQHandler(&fdi_i2cs.i2c_handle);
#endif
}

#ifdef FDI_I2CS_DMA

void DMA1_Channel6_IRQHandler(void) {
    HAL_DMA_IRQHandler(fdi_i2cs.i2c_handle.hdmatx);
}

void DMA1_Channel7_IRQHandler(void) {
    HAL_DMA_IRQHandler(fdi_i2cs.i2c_handle.hdmarx);
}

#endif