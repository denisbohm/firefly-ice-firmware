#include "fd_event.h"
#include "fd_processor.h"

#include <em_gpio.h>

#include <string.h>

fd_event_callback_t fd_event_callbacks[32];
volatile uint32_t fd_event_pending;

void fd_event_initialize(void) {
    for (int i = 0; i < 32; ++i) {
        fd_event_callbacks[i] = 0;
    }
    fd_event_pending = 0;
}

void fd_event_set_callback(uint32_t id, fd_event_callback_t callback) {
    fd_event_callbacks[id] = callback;
}

void fd_event_set(uint32_t id) {
    fd_event_pending |= 1 << id;
}

void GPIO_EVEN_IRQHandler(void) {
    uint32_t interrupts = GPIO_IntGet() & 0xaaaaaaaa;
    GPIO_IntClear(interrupts);
    if (interrupts & (1 << MAG_INT_PIN)) { // A.10
        fd_event_set(FD_EVENT_MAG_INT);
    }
    if (interrupts & (1 << NRF_RDYN_PIN)) { // D.4
        fd_event_set(FD_EVENT_NRF_RDYN);
    }
    if (interrupts & (1 << ACC_INT_PIN)) { // A.4
        fd_event_set(FD_EVENT_ACC_INT);
    }
    if (interrupts & (1 << CHG_STAT_PIN)) { // F.2
        fd_event_set(FD_EVENT_CHG_STAT);
    }
    if (interrupts & (1 << I2C0_INT_PIN)) { // C.8
        fd_event_set(FD_EVENT_CHG_STAT);
    }
}

void GPIO_ODD_IRQHandler(void) {
    uint32_t interrupts = GPIO_IntGet() & 0x55555555;
    GPIO_IntClear(interrupts);
    if (interrupts & (1 << CHG_STAT_PIN)) { // C.9
        fd_event_set(FD_EVENT_CHG_STAT);
    }
}

void fd_event_process(void) {
    fd_interrupts_disable();
    uint32_t pending = fd_event_pending;
    fd_interrupts_enable();

    uint32_t id = 0;
    while (pending) {
        uint32_t mask = 1 << id;
        if (pending & mask) {
            pending &= ~mask;
            fd_event_callback_t callback = fd_event_callbacks[id];
            if (callback) {
                (*callback)();
            }
        }
    }
}

