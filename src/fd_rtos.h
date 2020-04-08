#ifndef FD_RTOS_H
#define FD_RTOS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*fd_rtos_entry_point_t)(void);

typedef struct {
    uint32_t value;
    void *task;
} fd_rtos_condition_t;

void fd_rtos_initialize(void);

void fd_rtos_add_task(fd_rtos_entry_point_t entry_point, void *stack, size_t stack_size, uint32_t priority);

void fd_rtos_yield(void);

void fd_rtos_run(void);
void fd_rtos_deep_sleep(void);

uint32_t fd_rtos_interrupt_disable(void);
void fd_rtos_interrupt_enable(uint32_t state);

void fd_rtos_condition_initialize(fd_rtos_condition_t *condition);
void fd_rtos_condition_lock(fd_rtos_condition_t *condition);
void fd_rtos_condition_wait(fd_rtos_condition_t *condition, uint32_t state);
void fd_rtos_condition_signal(fd_rtos_condition_t *condition);
void fd_rtos_condition_unlock(fd_rtos_condition_t *condition);

void fd_rtos_delay(float sleep);

#endif
