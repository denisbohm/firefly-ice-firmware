#ifndef FD_EVENT_H
#define FD_EVENT_H

#include "fd_timing.h"

#include <stdbool.h>
#include <stdint.h>

#define FD_EVENT_NRF_RDYN (1 << 0)
#define FD_EVENT_CHG_STAT (1 << 1)
#define FD_EVENT_ACC_INT (1 << 2)
#define FD_EVENT_MAG_INT (1 << 3)
#define FD_EVENT_I2C0_INT (1 << 4)
#define FD_EVENT_ADC_TEMPERATURE (1 << 5)
#define FD_EVENT_ADC_BATTERY_VOLTAGE (1 << 6)
#define FD_EVENT_ADC_CHARGE_CURRENT (1 << 7)
#define FD_EVENT_RTC_TICK (1 << 8)
#define FD_EVENT_RTC_COUNTDOWN (1 << 9)
#define FD_EVENT_TIMER_SCHEDULE (1 << 10)
#define FD_EVENT_USB_STATE (1 << 11)
#define FD_EVENT_USB_TRANSFER (1 << 12)
#define FD_EVENT_BLE_STATE (1 << 13)
#define FD_EVENT_BLE_DATA_CREDITS (1 << 14)
#define FD_EVENT_BLE_SYSTEM_CREDITS (1 << 15)
#define FD_EVENT_BLE_STEP (1 << 16)
#define FD_EVENT_COMMAND (1 << 17)
#define FD_EVENT_LOCK_STATE (1 << 18)
#define FD_EVENT_USB_POWER (1 << 19)
#define FD_EVENT_NOTIFY (1 << 20)

typedef void (*fd_event_callback_t)(void);

typedef bool (*fd_event_em2_check_t)(void);

void fd_event_initialize(void);

void fd_event_add_em2_check(fd_event_em2_check_t em2_check);

void fd_event_add_callback_with_identifier(uint32_t events, fd_event_callback_t callback, const char *identifier);
#define fd_event_add_callback(events, callback) fd_event_add_callback_with_identifier(events, callback, #callback)

void fd_event_set_exclusive(uint32_t events);
void fd_event_set(uint32_t events);

bool fd_event_process_pending(void);

void fd_event_process(void);

fd_timing_iterator_t fd_event_timing_iterator(void);

#endif
