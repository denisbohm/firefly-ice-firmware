#include "fd_rtc.h"

#include "fd_hal_processor.h"
#include "fd_hal_rtc.h"

#include "fd_nrf5.h"

// Use compare 1 to trigger rtc ticks, so that we can use a counter prescaler of 1.
// This lets us read the counter register to get an "accurate" time (~30.5 us resolution).

#define FD_RTC_CLOCK_FREQUENCY 32768UL

#define FD_RTC_MAX_TASK_DELAY 47

typedef struct {
    const fd_rtc_t *rtc;
    IRQn_Type irqn;
    uint32_t count_per_tick;
    uint32_t correction_ticks;
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
        info->count_per_tick = FD_RTC_CLOCK_FREQUENCY / rtc->ticks_per_second;
        info->correction_ticks = 0;
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

    rtc->EVTENCLR = RTC_EVTEN_COMPARE1_Msk;
    rtc->INTENCLR = RTC_INTENSET_COMPARE1_Msk;

    rtc->TASKS_STOP = 1;
    nrf_delay_us(FD_RTC_MAX_TASK_DELAY);

    rtc->TASKS_CLEAR = 1;
    nrf_delay_us(FD_RTC_MAX_TASK_DELAY);

    rtc->CC[1] = 0;
}

#define FD_RTC_GPIOTE_CHANNEL 0
#define FD_RTC_PPI_CHANNEL 0

void fd_hal_rtc_enable_pin_output(const fd_rtc_t *fd_rtc, fd_gpio_t gpio) {
    NRF_RTC_Type *rtc = (NRF_RTC_Type *)fd_rtc->instance;

    // To use compare 1 event to toggle the gpio.
    NRF_GPIOTE->CONFIG[FD_RTC_GPIOTE_CHANNEL] =
        (GPIOTE_CONFIG_MODE_Task       << GPIOTE_CONFIG_MODE_Pos)     | 
        (GPIOTE_CONFIG_OUTINIT_Low     << GPIOTE_CONFIG_OUTINIT_Pos)  |
        (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) | 
        (gpio.port                     << GPIOTE_CONFIG_PORT_Pos)     |
        (gpio.pin                      << GPIOTE_CONFIG_PSEL_Pos);

    // Connect RTC1 tick event to GPIOTE toggle task
#if 1
    sd_ppi_channel_assign(FD_RTC_PPI_CHANNEL, &rtc->EVENTS_COMPARE[1], &NRF_GPIOTE->TASKS_OUT[0]);
    sd_ppi_channel_enable_set(1 << FD_RTC_PPI_CHANNEL);
#else
    NRF_PPI->CH[FD_RTC_PPI_CHANNEL].EEP = (uint32_t) &rtc->EVENTS_COMPARE[1];
    NRF_PPI->CH[FD_RTC_PPI_CHANNEL].TEP = (uint32_t) &NRF_GPIOTE->TASKS_OUT[0];
    NRF_PPI->CHEN |= 1 << FD_RTC_PPI_CHANNEL;
#endif
}

void fd_hal_rtc_disable_pin_output(const fd_rtc_t *fd_rtc, fd_gpio_t gpio) {
    NRF_GPIOTE->CONFIG[FD_RTC_GPIOTE_CHANNEL] = 0;
#if 1
    sd_ppi_channel_enable_clr(1 << FD_RTC_PPI_CHANNEL);
//?    nrf_drv_ppi_channel_free(FD_RTC_PPI_CHANNEL);
#else
    NRF_PPI->CHEN &= ~(1 << FD_RTC_PPI_CHANNEL);
    NRF_PPI->CH[FD_RTC_PPI_CHANNEL].EEP = 0;
    NRF_PPI->CH[FD_RTC_PPI_CHANNEL].TEP = 0;
#endif
}

void fd_rtc_enable(const fd_rtc_t *fd_rtc) {
    NRF_RTC_Type *rtc = (NRF_RTC_Type *)fd_rtc->instance;
    fd_rtc_info_t *info = fd_rtc_get_info(fd_rtc->instance);

    rtc->EVTENSET = RTC_EVTEN_COMPARE1_Msk;
    rtc->INTENSET = RTC_INTENSET_COMPARE1_Msk;

    IRQn_Type irqn = info->irqn;
    NVIC_SetPriority(irqn, APP_IRQ_PRIORITY_HIGH);
    NVIC_ClearPendingIRQ(irqn);
    NVIC_EnableIRQ(irqn);

    info->last_counter = 0;
    rtc->TASKS_CLEAR = 1;
    nrf_delay_us(FD_RTC_MAX_TASK_DELAY);

    rtc->CC[1] = info->count_per_tick;

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
    bool events_compare_1 = rtc->EVENTS_COMPARE[1] != 0;

    // Clear all events (also unexpected ones)
    rtc->EVENTS_COMPARE[0] = 0;
    rtc->EVENTS_COMPARE[1] = 0;
    rtc->EVENTS_COMPARE[2] = 0;
    rtc->EVENTS_COMPARE[3] = 0;
    rtc->EVENTS_TICK       = 0;
    rtc->EVENTS_OVRFLW     = 0;

    if (events_compare_1) {
        fd_rtc_info_t *info = fd_rtc_get_info((uint32_t)rtc);
        if (info != 0) {
            uint32_t count_per_tick = info->count_per_tick;
            uint32_t ticks_per_correction = info->rtc->ticks_per_correction;
            if (ticks_per_correction) {
                if (++info->correction_ticks >= ticks_per_correction) {
                    info->correction_ticks = 0;
                    count_per_tick += info->rtc->correction_count;
                }
            }
            uint32_t counter = rtc->COUNTER;
            rtc->CC[1] = (counter & ~(count_per_tick - 1)) + count_per_tick;
            info->last_counter = counter;

            if (info->rtc->handler != 0) {
                info->rtc->handler();
            }
        }
    }
}

void RTC2_IRQHandler(void) {
    fd_rtc_handler(NRF_RTC2);
}
