#ifndef FD_PROCESSOR
#define FD_PROCESSOR

#include <stdint.h>

#define BAT_VDIV2EN_PORT_PIN gpioPortE, 9

#define LED1_PORT_PIN gpioPortE, 12
#define LED2_PORT_PIN gpioPortE, 13
#define LED3_PORT_PIN gpioPortE, 14
#define LED4_PORT_PIN gpioPortE, 15

#define LED_EN_PORT_PIN gpioPortA, 8
#define AUX_PWR_PORT_PIN gpioPortA, 9
#define MAG_INT_PIN 10
#define MAG_INT_PORT_PIN gpioPortA, 10

#define LED5_PORT_PIN gpioPortC, 0
#define LED6_PORT_PIN gpioPortC, 1
#define LED7_PORT_PIN gpioPortC, 2
#define LED8_PORT_PIN gpioPortC, 3

#define ACC_CSN_PORT_PIN gpioPortC, 6
#define NRF_PWR_PORT_PIN gpioPortC, 7
#define US0_LOCATION USART_ROUTE_LOCATION_LOC2
#define US0_CLK_PORT_PIN gpioPortC, 9
#define US0_MISO_PORT_PIN gpioPortC, 10
#define US0_MOSI_PORT_PIN gpioPortC, 11

#define SWD_CLK_PORT_PIN gpioPortF, 0
#define SWD_IO_PORT_PIN gpioPortF, 1
#define CHG_STAT_PIN 2
#define CHG_STAT_PORT_PIN gpioPortF, 2
#define PWR_HIGH_PORT_PIN gpioPortF, 5
#define USB_DM_PORT_PIN gpioPortF, 10
#define USB_DP_PORT_PIN gpioPortF, 11
#define PWR_MODE_PORT_PIN gpioPortF, 12

#define US1_LOCATION USART_ROUTE_LOCATION_LOC1
#define US1_MOSI_PORT_PIN gpioPortD, 0
#define US1_MISO_PORT_PIN gpioPortD, 1
#define US1_CLK_PORT_PIN gpioPortD, 2
#define NRF_REQN_PORT_PIN gpioPortD, 3
#define NRF_RDYN_PIN 4
#define NRF_RDYN_PORT_PIN gpioPortD, 4
#define NRF_RESETN_PORT_PIN gpioPortD, 5
#define BAT_VDIV2_PORT_PIN gpioPortD, 6
#define CHG_RATE_PORT_PIN gpioPortD, 7
#define ACC_INT_PIN 8
#define ACC_INT_PORT_PIN gpioPortD, 8

#define LFXTAL_P_PORT_PIN gpioPortB, 7
#define LFXTAL_N_PORT_PIN gpioPortB, 8
#define I2C1_LOCATION I2C_ROUTE_LOCATION_LOC1
#define I2C1_SDA_PORT_PIN gpioPortB, 11
#define I2C1_SCL_PORT_PIN gpioPortB, 12
#define HFXTAL_P_PORT_PIN gpioPortB, 13
#define HFXTAL_N_PORT_PIN gpioPortB, 14

void fd_processor_initialize(void);

void fd_delay_ms(uint32_t ms);

void fd_interrupts_disable(void);
void fd_interrupts_enable(void);

#endif