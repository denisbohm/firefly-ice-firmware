#include "fd_event.h"
#include "fd_pins.h"

#include <em_gpio.h>

void fd_pins_events_initialize(void) {
    NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
    NVIC_EnableIRQ(GPIO_EVEN_IRQn);
    NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
    NVIC_EnableIRQ(GPIO_ODD_IRQn);
}

void GPIO_EVEN_IRQHandler(void) {
    uint32_t interrupts = GPIO_IntGet() & 0x55555555;
    GPIO_IntClear(interrupts);
    if (interrupts & (1 << MAG_INT_PIN)) { // A.10
        fd_event_set(FD_EVENT_MAG_INT);
    }
    if (interrupts & (1 << NRF_RDYN_PIN)) { // D.4
        fd_event_set(FD_EVENT_NRF_RDYN);
    }
    if (interrupts & (1 << I2C0_INT_PIN)) { // C.8
        fd_event_set(FD_EVENT_I2C0_INT);
    }
}

void GPIO_ODD_IRQHandler(void) {
    uint32_t interrupts = GPIO_IntGet() & 0xaaaaaaaa;
    GPIO_IntClear(interrupts);
    if (interrupts & (1 << ACC_INT_PIN)) { // A.5
        fd_event_set(FD_EVENT_ACC_INT);
    }
    if (interrupts & (1 << CHG_STAT_PIN)) { // C.9
        fd_event_set(FD_EVENT_CHG_STAT);
    }
}
