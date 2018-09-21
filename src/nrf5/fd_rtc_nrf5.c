#include "fd_rtc.h"

#include "fd_hal_processor.h"

#include "fd_nrf5.h"

// Use compare 1 to trigger rtc ticks, so that we can use a counter prescaler of 1.
// This lets us read the counter register to get an "accurate" time (~30.5 us resolution).

#define FD_RTC_CLOCK_FREQUENCY 32768UL

#define FD_RTC_MAX_TASK_DELAY 47

typedef struct {
    const fd_rtc_t *rtc;
    IRQn_Type irqn;
    uint32_t counter;
    uint32_t last_counter;
} fd_rtc_info_t;

#define fd_rtc_size 3
fd_rtc_info_t fd_rtc_infos[fd_rtc_size];
uint32_t fd_rtc_count;

static
IRQn_Type fd_rtc_get_irqn(const fd_rtc_t *fd_rtc) {
    switch (fd_rtc->instance) {
        case NRF_RTC0_BASE:
            return RTC0_IRQn;
        case NRF_RTC1_BASE:
            return RTC1_IRQn;
        case NRF_RTC2_BASE:
            return RTC2_IRQn;
    }
    return RTC2_IRQn;
}

void fd_rtc_initialize(
    const fd_rtc_t *rtcs, uint32_t rtc_count
) {
    fd_rtc_count = rtc_count < fd_rtc_size ? rtc_count : fd_rtc_size;
    for (uint32_t i = 0; i < rtc_count; ++i) {
        const fd_rtc_t *rtc = &rtcs[i];
        fd_rtc_info_t *info = &fd_rtc_infos[i];
        info->rtc = rtc;
        info->irqn = fd_rtc_get_irqn(rtc);
        info->counter = FD_RTC_CLOCK_FREQUENCY / rtc->ticks_per_second;
        info->last_counter = 0;
        fd_rtc_disable(rtc);
    }
}

static
fd_rtc_info_t *fd_rtc_get_info(uint32_t instance) {
    for (uint32_t i = 0; i < fd_rtc_count; ++i) {
        fd_rtc_info_t *info = &fd_rtc_infos[i];
        if (info->rtc->instance == instance) {
            return info;
        }
    }
    return 0;
}


void fd_rtc_disable(const fd_rtc_t *fd_rtc) {
    NRF_RTC_Type *rtc = (NRF_RTC_Type *)fd_rtc->instance;
    fd_rtc_info_t *info = fd_rtc_get_info(fd_rtc->instance);

    NVIC_DisableIRQ(info->irqn);

    rtc->EVTENCLR = RTC_EVTEN_COMPARE0_Msk | RTC_EVTEN_COMPARE1_Msk;
    rtc->INTENCLR = RTC_INTENSET_COMPARE0_Msk | RTC_INTENSET_COMPARE1_Msk;

    rtc->TASKS_STOP = 1;
    nrf_delay_us(FD_RTC_MAX_TASK_DELAY);

    rtc->TASKS_CLEAR = 1;
    nrf_delay_us(FD_RTC_MAX_TASK_DELAY);

    rtc->CC[1] = 0;
}

void fd_rtc_enable(const fd_rtc_t *fd_rtc) {
    NRF_RTC_Type *rtc = (NRF_RTC_Type *)fd_rtc->instance;
    fd_rtc_info_t *info = fd_rtc_get_info(fd_rtc->instance);

    rtc->EVTENSET = RTC_EVTEN_COMPARE0_Msk | RTC_EVTEN_COMPARE1_Msk;
    rtc->INTENSET = RTC_INTENSET_COMPARE0_Msk | RTC_INTENSET_COMPARE1_Msk;

    IRQn_Type irqn = info->irqn;
    NVIC_SetPriority(irqn, APP_IRQ_PRIORITY_HIGH);
    NVIC_ClearPendingIRQ(irqn);
    NVIC_EnableIRQ(irqn);

    info->last_counter = 0;
    rtc->TASKS_CLEAR = 1;
    nrf_delay_us(FD_RTC_MAX_TASK_DELAY);

    rtc->CC[1] = info->counter;

    rtc->TASKS_START = 1;
    nrf_delay_us(FD_RTC_MAX_TASK_DELAY);
}

bool fd_rtc_is_enabled(const fd_rtc_t *fd_rtc) {
    NRF_RTC_Type *rtc = (NRF_RTC_Type *)fd_rtc->instance;

    return rtc->CC[1] != 0;
}

uint32_t fd_rtc_get_subticks(const fd_rtc_t *fd_rtc) {
    NRF_RTC_Type *rtc = (NRF_RTC_Type *)fd_rtc->instance;
    fd_rtc_info_t *info = fd_rtc_get_info(fd_rtc->instance);
    fd_hal_processor_interrupts_disable();
    uint32_t subticks = rtc->COUNTER - info->last_counter;
    fd_hal_processor_interrupts_enable();
    return subticks;
}

void fd_rtc_handler(NRF_RTC_Type *rtc) {
    if (rtc->EVENTS_COMPARE[1]) {
        fd_rtc_info_t *info = fd_rtc_get_info((uint32_t)rtc);
        if (info != 0) {
            uint32_t counter = rtc->COUNTER;
            rtc->CC[1] = (counter & ~(info->counter - 1)) + info->counter;
            info->last_counter = counter;

            if (info->rtc->handler != 0) {
                info->rtc->handler();
            }
        }
    }

    // Clear all events (also unexpected ones)
    rtc->EVENTS_COMPARE[0] = 0;
    rtc->EVENTS_COMPARE[1] = 0;
    rtc->EVENTS_COMPARE[2] = 0;
    rtc->EVENTS_COMPARE[3] = 0;
    rtc->EVENTS_TICK       = 0;
    rtc->EVENTS_OVRFLW     = 0;
}

void RTC2_IRQHandler(void) {
    fd_rtc_handler(NRF_RTC2);
}
