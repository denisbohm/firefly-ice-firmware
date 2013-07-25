#include "fd_indicator.h"
#include "fd_lp55231.h"
#include "fd_processor.h"
#include "fd_timer.h"

#include <em_cmu.h>
#include <em_gpio.h>
#include <em_timer.h>

// D0 (r) PC1 TIM0_CC2 #4
// D4 (r) PC0 TIM0_CC1 #4
// D5 (usb.o) PE9
// D6 (usb.g) PA15 TIM3_CC2 #0

static
fd_timer_t override_timer;

void fd_indicator_set_usb(uint8_t orange, uint8_t green) {
    TIMER_CompareSet(TIMER3, /* channel */ 1, orange);
    TIMER_CompareSet(TIMER3, /* channel */ 2, green);
}

void fd_indicator_set_d0(uint8_t value) {
    TIMER_CompareSet(
    , /* channel */ 2, value);
}

void fd_indicator_set_d1(uint8_t red, uint8_t green, uint8_t blue) {
    fd_lp55231_set_led_pwm(8, red);
    fd_lp55231_set_led_pwm(4, green);
    fd_lp55231_set_led_pwm(5, blue);
}

void fd_indicator_set_d2(uint8_t red, uint8_t green, uint8_t blue) {
    fd_lp55231_set_led_pwm(7, red);
    fd_lp55231_set_led_pwm(2, green);
    fd_lp55231_set_led_pwm(3, blue);
}

void fd_indicator_set_d3(uint8_t red, uint8_t green, uint8_t blue) {
    fd_lp55231_set_led_pwm(6, red);
    fd_lp55231_set_led_pwm(0, green);
    fd_lp55231_set_led_pwm(1, blue);
}

void fd_indicator_set_d4(uint8_t value) {
    TIMER_CompareSet(TIMER0, /* channel */ 1, value);
}

static
void override_callback(void) {
// !!! these should go back to their desired values, not zero -denis
    fd_indicator_set_usb(0, 0);

    fd_indicator_set_d0(0);
    fd_indicator_set_d4(0);

    fd_indicator_set_d1(0, 0, 0);
    fd_indicator_set_d2(0, 0, 0);
    fd_indicator_set_d3(0, 0, 0);
}

void fd_indicator_initialize(void) {
    fd_timer_add(&override_timer, override_callback);
}

#define TOP 255

void fd_indicator_wake(void) {
    CMU_ClockEnable(cmuClock_TIMER0, true);

    TIMER_InitCC_TypeDef timer_initcc = TIMER_INITCC_DEFAULT;
    timer_initcc.cmoa = timerOutputActionToggle;
    timer_initcc.mode = timerCCModePWM;
    TIMER_InitCC(TIMER0, /* channel */ 1, &timer_initcc);
    TIMER_InitCC(TIMER0, /* channel */ 2, &timer_initcc);

    TIMER0->ROUTE = TIMER_ROUTE_CC1PEN | TIMER_ROUTE_CC2PEN | TIMER_ROUTE_LOCATION_LOC4;

    TIMER_TopSet(TIMER0, TOP);
    TIMER_CompareSet(TIMER0, /* channel */ 1, TOP);
    TIMER_CompareSet(TIMER0, /* channel */ 2, TOP);

    CMU_ClockEnable(cmuClock_TIMER3, true);

    TIMER_InitCC(TIMER3, /* channel */ 2, &timer_initcc);

    TIMER3->ROUTE = TIMER_ROUTE_CC2PEN | TIMER_ROUTE_LOCATION_LOC0;

    TIMER_TopSet(TIMER3, TOP);
    TIMER_CompareSet(TIMER3, /* channel */ 1, TOP);
    TIMER_CompareSet(TIMER3, /* channel */ 2, TOP);

    TIMER_IntEnable(TIMER3, TIMER_IF_CC1 | TIMER_IF_OF);

    NVIC_EnableIRQ(TIMER3_IRQn);
}

void fd_indicator_sleep(void) {
    TIMER0->ROUTE = 0;

    CMU_ClockEnable(cmuClock_TIMER0, false);

    TIMER3->ROUTE = 0;

    CMU_ClockEnable(cmuClock_TIMER3, false);
}

void TIMER3_IRQHandler(void) {
    uint32_t timer_if = TIMER_IntGet(TIMER3);
    TIMER_IntClear(TIMER3, TIMER_IF_CC1 | TIMER_IF_OF);

    if (timer_if & TIMER_IF_CC1) {
        GPIO_PinOutClear(LED5_PORT_PIN);
    } else {
        GPIO_PinOutSet(LED5_PORT_PIN);
    }
}

void fd_indicator_override(fd_indicator_state_t *state, fd_time_t duration) {
    fd_indicator_set_usb(state->usb.o, state->usb.g);

    fd_indicator_set_d0(state->d0.r);
    fd_indicator_set_d4(state->d4.r);

    fd_indicator_set_d1(state->d1.r, state->d1.g, state->d1.b);
    fd_indicator_set_d2(state->d2.r, state->d2.g, state->d2.b);
    fd_indicator_set_d3(state->d3.r, state->d3.g, state->d3.b);

    fd_timer_start(&override_timer, duration);
}