#include "fd_event.h"
#include "fd_processor.h"
#include "fd_usb.h"

#include <em_emu.h>
#include <em_gpio.h>
#include <em_wdog.h>

#include <string.h>

typedef struct {
    uint32_t events;
    fd_event_callback_t callback;
} fd_event_item_t;

#define ITEM_LIMIT 32

static
fd_event_item_t fd_event_items[ITEM_LIMIT];
static
uint32_t fd_event_item_count;

volatile uint32_t fd_event_pending;
volatile uint32_t fd_event_mask;

void fd_event_initialize(void) {
    fd_event_item_count = 0;
    fd_event_pending = 0;
    fd_event_mask = 0;

    NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
    NVIC_EnableIRQ(GPIO_EVEN_IRQn);
    NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
    NVIC_EnableIRQ(GPIO_ODD_IRQn);
}

void fd_event_add_callback(uint32_t events, fd_event_callback_t callback) {
    if (fd_event_item_count >= ITEM_LIMIT) {
        // !!! log something
        return;
    }

    fd_event_item_t *item = &fd_event_items[fd_event_item_count++];
    item->events = events;
    item->callback = callback;
}

void fd_event_set(uint32_t events) {
    fd_event_pending |= events;
}

void fd_event_mask_set(uint32_t events) {
    fd_event_mask |= events;
}

void fd_event_mask_clear(uint32_t events) {
    fd_event_mask &= ~events;
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
    if (interrupts & (1 << ACC_INT_PIN)) { // A.4
        fd_event_set(FD_EVENT_ACC_INT);
    }
    if (interrupts & (1 << I2C0_INT_PIN)) { // C.8
        fd_event_set(FD_EVENT_I2C0_INT);
    }
}

void GPIO_ODD_IRQHandler(void) {
    uint32_t interrupts = GPIO_IntGet() & 0xaaaaaaaa;
    GPIO_IntClear(interrupts);
    if (interrupts & (1 << CHG_STAT_PIN)) { // C.9
        fd_event_set(FD_EVENT_CHG_STAT);
    }
}

void fd_event_process(void) {
    fd_interrupts_disable();
    uint32_t pending = fd_event_pending & ~fd_event_mask;
    fd_event_pending = 0;
    fd_interrupts_enable();

    WDOG_Feed();

    if (pending) {
        for (uint32_t i = 0; i < fd_event_item_count; ++i) {
            fd_event_item_t *item = &fd_event_items[i];
            if (item->events & pending) {
                (*item->callback)();
            }
        }
    } else {
        if (fd_usb_is_safe_to_enter_em2()) {
            EMU_EnterEM2(true);
        }
    }
}

