#ifndef FD_EVENT_H
#define FD_EVENT_H

#include <stdint.h>

#define FD_EVENT_NRF_RDYN (1 << 0)
#define FD_EVENT_CHG_STAT (1 << 1)
#define FD_EVENT_ACC_INT (1 << 2)
#define FD_EVENT_MAG_INT (1 << 3)
#define FD_EVENT_RTC_COUNTDOWN (1 << 4)
#define FD_EVENT_TIMER_SCHEDULE (1 << 5)
#define FD_EVENT_USB_STATE (1 << 6)
#define FD_EVENT_USB_TRANSFER (1 << 7)
#define FD_EVENT_BLE_STATE (1 << 8)
#define FD_EVENT_BLE_DATA_CREDITS (1 << 9)
#define FD_EVENT_BLE_SYSTEM_CREDITS (1 << 10)

typedef void (*fd_event_callback_t)(void);

void fd_event_initialize(void);
void fd_event_add_callback(uint32_t events, fd_event_callback_t callback);

void fd_event_set(uint32_t events);

void fd_event_process(void);

#endif