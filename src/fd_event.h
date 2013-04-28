#ifndef FD_EVENT_H
#define FD_EVENT_H

#include <stdint.h>

#define FD_EVENT_NRF_RDYN 0
#define FD_EVENT_CHG_STAT 1
#define FD_EVENT_ACC_INT 2
#define FD_EVENT_MAG_INT 3

typedef void (*fd_event_callback_t)(void);

void fd_event_initialize(void);
void fd_event_set_callback(uint32_t id, fd_event_callback_t callback);

void fd_event_set(uint32_t id);

void fd_event_process(void);

#endif