#ifndef FD_RTOS_H
#define FD_RTOS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*fd_rtos_entry_point_t)(void);

void fd_rtos_initialize(void);

void fd_rtos_add_task(fd_rtos_entry_point_t entry_point, void *stack, size_t stack_size, uint32_t priority);

void fd_rtos_yield(void);

void fd_rtos_run(void);

void fd_rtos_test(void);

#endif