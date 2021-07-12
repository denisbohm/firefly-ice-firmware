#ifndef stm32l4xx_hal_conf_h
#define stm32l4xx_hal_conf_h

#include "stm32_assert.h"

#include "stm32l4xx_hal_cortex.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal.h"

#include "stm32l4xx_hal_dma.h"
#include "stm32l4xx_hal_adc.h"
#include "stm32l4xx_hal_adc_ex.h"
#include "stm32l4xx_hal_dac.h"
#include "stm32l4xx_hal_dac_ex.h"
#include "stm32l4xx_hal_dma.h"
#include "stm32l4xx_hal_dma_ex.h"
#include "stm32l4xx_hal_flash.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_hal_i2c.h"
#include "stm32l4xx_hal_i2c_ex.h"
#include "stm32l4xx_hal_rcc.h"
#include "stm32l4xx_hal_rcc_ex.h"
#include "stm32l4xx_hal_pcd.h"
#include "stm32l4xx_hal_pwr.h"
#include "stm32l4xx_hal_pwr_ex.h"
#include "stm32l4xx_hal_tim.h"
#include "stm32l4xx_hal_tim_ex.h"

#define HAL_ADC_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DAC_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_MODULE_ENABLED
#define HAL_PCD_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED

#define HSE_VALUE               8000000U
#define HSE_STARTUP_TIMEOUT         100U
#define LSE_VALUE                 32768U
#define LSE_STARTUP_TIMEOUT        5000U
#define EXTERNAL_SAI1_CLOCK_VALUE 48000U
#define EXTERNAL_SAI2_CLOCK_VALUE 48000U

#define MSI_VALUE    4000000U
#define HSI_VALUE   16000000U
#define HSI48_VALUE 48000000U
#define LSI_VALUE      32000U

#define TICK_INT_PRIORITY 0x0FU

#define USE_USB_CLKSOURCE_CRSHSI48 1

#endif
