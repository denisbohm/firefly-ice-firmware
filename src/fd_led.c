#include "fd_event.h"
#include "fd_hal_processor.h"
#include "fd_led.h"
#include "fd_lp55231.h"
#include "fd_pins.h"
#include "fd_timer.h"

#include <em_cmu.h>
#include <em_gpio.h>
#include <em_timer.h>

#include <string.h>

// D0 (r) PC1 TIM0_CC2 #4
// D4 (r) PC0 TIM0_CC1 #4
// D5 (usb.o) PE9
// D6 (usb.g) PA15 TIM3_CC2 #0

#define TOP 0xff00

static
fd_timer_t override_timer;

static
uint32_t active_leds;

static
fd_hal_ui_led_state_t led_state;

static
void fd_led_restore(void) {
    fd_led_set_usb(led_state.usb.o, led_state.usb.g);

    fd_led_set_d0(led_state.d0.r);
    fd_led_set_d4(led_state.d4.r);

    fd_led_set_d1(led_state.d1.r, led_state.d1.g, led_state.d1.b);
    fd_led_set_d2(led_state.d2.r, led_state.d2.g, led_state.d2.b);
    fd_led_set_d3(led_state.d3.r, led_state.d3.g, led_state.d3.b);
}

static
void override_callback(void) {
    fd_led_restore();
}

static bool fd_led_em2_check(void) {
    return active_leds == 0;
}

void fd_led_initialize(void) {
    fd_timer_add(&override_timer, override_callback);

    TIMER_CompareSet(TIMER0, /* channel */ 1, TOP);
    TIMER_CompareSet(TIMER0, /* channel */ 2, TOP);

    TIMER_CompareSet(TIMER3, /* channel */ 1, TOP);
    TIMER_CompareSet(TIMER3, /* channel */ 2, TOP);

    fd_led_sleep();

    fd_event_add_em2_check(fd_led_em2_check);
}

void fd_led_change_before(uint32_t n, uint32_t value) {
    uint32_t active = active_leds;
    bool was_active = (active != 0);
    if (value != 0) {
        active |= (1 << n);
    } else {
        active &= ~(1 << n);
    }
    bool is_active = (active != 0);
    if (!was_active && is_active) {
        fd_led_wake();
    }
}

void fd_led_change_after(uint32_t n, uint32_t value) {
    bool was_active = (active_leds != 0);
    if (value != 0) {
        active_leds |= (1 << n);
    } else {
        active_leds &= ~(1 << n);
    }
    bool is_active = (active_leds != 0);
    if (was_active && !is_active) {
        fd_led_sleep();
    }
}

void fd_led_set_usb(uint8_t orange, uint8_t green) {
    led_state.usb.o = orange;
    led_state.usb.g = green;
    if (override_timer.active) {
        return;
    }

    fd_led_change_before(5, (orange << 8) | green);

    // to distinguish overflow and compare interrupts unambiguously -denis
    if (orange == 0xff) {
        orange = 0xfe;
    }

    TIMER_CompareSet(TIMER3, /* channel */ 1, (~orange) << 8);
    TIMER_CompareSet(TIMER3, /* channel */ 2, (~green) << 8);

    fd_led_change_after(5, (orange << 8) | green);
}

void fd_led_set_d0(uint8_t value) {
    led_state.d0.r = value;
    if (override_timer.active) {
        return;
    }

    fd_led_change_before(0, value);

    TIMER_CompareSet(TIMER0, /* channel */ 2, (~value) << 8);

    fd_led_change_after(0, value);
}

void fd_led_set_d1(uint8_t red, uint8_t green, uint8_t blue) {
    led_state.d1.r = red;
    led_state.d1.g = green;
    led_state.d1.b = blue;
    if (override_timer.active) {
        return;
    }

    fd_led_change_before(1, (red << 24) | (green << 16) | blue);

    fd_lp55231_set_led_pwm(9, red);
    fd_lp55231_set_led_pwm(6, green);
    fd_lp55231_set_led_pwm(5, blue);

    fd_led_change_after(1, (red << 24) | (green << 16) | blue);
}

void fd_led_set_d2(uint8_t red, uint8_t green, uint8_t blue) {
    led_state.d2.r = red;
    led_state.d2.g = green;
    led_state.d2.b = blue;
    if (override_timer.active) {
        return;
    }

    fd_led_change_before(2, (red << 24) | (green << 16) | blue);

    fd_lp55231_set_led_pwm(8, red);
    fd_lp55231_set_led_pwm(4, green);
    fd_lp55231_set_led_pwm(3, blue);

    fd_led_change_after(2, (red << 24) | (green << 16) | blue);
}

void fd_led_set_d3(uint8_t red, uint8_t green, uint8_t blue) {
    led_state.d3.r = red;
    led_state.d3.g = green;
    led_state.d3.b = blue;
    if (override_timer.active) {
        return;
    }

    fd_led_change_before(3, (red << 24) | (green << 16) | blue);

    fd_lp55231_set_led_pwm(7, red);
    fd_lp55231_set_led_pwm(2, green);
    fd_lp55231_set_led_pwm(1, blue);

    fd_led_change_after(3, (red << 24) | (green << 16) | blue);
}

void fd_led_set_d4(uint8_t value) {
    led_state.d4.r = value;
    if (override_timer.active) {
        return;
    }

    fd_led_change_before(4, value);

    TIMER_CompareSet(TIMER0, /* channel */ 1, (~value) << 8);

    fd_led_change_after(4, value);
}

void fd_led_wake(void) {
    fd_lp55231_power_on();
    fd_lp55231_wake();

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

    TIMER_Init_TypeDef timer_init = TIMER_INIT_DEFAULT;
    TIMER_Init(TIMER0, &timer_init);

    CMU_ClockEnable(cmuClock_TIMER3, true);

    TIMER_InitCC(TIMER3, /* channel */ 1, &timer_initcc);
    TIMER_InitCC(TIMER3, /* channel */ 2, &timer_initcc);

    TIMER3->ROUTE = TIMER_ROUTE_CC2PEN | TIMER_ROUTE_LOCATION_LOC0;

    TIMER_TopSet(TIMER3, TOP);
    TIMER_CompareSet(TIMER3, /* channel */ 1, TOP);
    TIMER_CompareSet(TIMER3, /* channel */ 2, TOP);

    TIMER_IntEnable(TIMER3, TIMER_IF_CC1 | TIMER_IF_OF);

    NVIC_EnableIRQ(TIMER3_IRQn);

    TIMER_Init(TIMER3, &timer_init);
}

void fd_led_sleep(void) {
    active_leds = 0;
    memset(&led_state, 0, sizeof(led_state));

    fd_timer_stop(&override_timer);

    fd_lp55231_sleep();
    fd_lp55231_power_off();

    TIMER0->ROUTE = 0;

    CMU_ClockEnable(cmuClock_TIMER0, false);

    TIMER3->ROUTE = 0;

    CMU_ClockEnable(cmuClock_TIMER3, false);

    GPIO_PinOutSet(LED0_PORT_PIN);
    GPIO_PinOutSet(LED4_PORT_PIN);
    GPIO_PinOutSet(LED5_PORT_PIN);
    GPIO_PinOutSet(LED6_PORT_PIN);
}

void TIMER3_IRQHandler(void) {
    uint32_t timer_if = TIMER_IntGet(TIMER3) & (TIMER_IF_CC1 | TIMER_IF_OF);
    TIMER_IntClear(TIMER3, TIMER_IF_CC1 | TIMER_IF_OF);

    if (timer_if & TIMER_IF_OF) {
        GPIO_PinOutSet(LED5_PORT_PIN);
    } else {
        GPIO_PinOutClear(LED5_PORT_PIN);
    }
}

void fd_led_override(fd_hal_ui_led_state_t *state, fd_time_t duration) {
    fd_hal_ui_led_state_t saved_state = led_state;

    fd_led_set_usb(state->usb.o, state->usb.g);
    fd_led_set_d0(state->d0.r);
    fd_led_set_d4(state->d4.r);
    fd_led_set_d1(state->d1.r, state->d1.g, state->d1.b);
    fd_led_set_d2(state->d2.r, state->d2.g, state->d2.b);
    fd_led_set_d3(state->d3.r, state->d3.g, state->d3.b);

    led_state = saved_state;
    fd_timer_start(&override_timer, duration);
}