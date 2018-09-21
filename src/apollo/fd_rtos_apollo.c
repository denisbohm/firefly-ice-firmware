#include "fd_rtos.h"

#include "apollo2.h"
#include <am_mcu_apollo.h>

#include <string.h>

typedef struct {
    void *sp;
    bool runnable;
} fd_rtos_task_t;

typedef struct {
    uint32_t registers[8]; // r4-r11
} fd_rtos_software_stack_frame_t;

typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
} fd_rtos_hardware_stack_frame_t;

typedef struct {
    fd_rtos_software_stack_frame_t software;
    fd_rtos_hardware_stack_frame_t hardware;
} fd_rtos_stack_frame_t;

#define fd_rtos_task_size 8
fd_rtos_task_t fd_rtos_tasks[fd_rtos_task_size];
uint32_t fd_rtos_task_count;
uint32_t fd_rtos_task_index;
uint32_t fd_rtos_assertion_failure_count;

void fd_rtos_assert(bool value) {
    if (!value) {
        ++fd_rtos_assertion_failure_count;
    }
}

void fd_rtos_initialize(void) {
    memset(fd_rtos_tasks, 0, sizeof(fd_rtos_tasks));
    fd_rtos_task_count = 0;
    fd_rtos_task_index = 0;
    fd_rtos_assertion_failure_count = 0;

    NVIC_SetPriority(PendSV_IRQn, 1);

    am_hal_stimer_config(AM_HAL_STIMER_HFRC_3MHZ | AM_HAL_STIMER_CFG_COMPARE_A_ENABLE);
    NVIC_ClearPendingIRQ(STIMER_CMPR0_IRQn);
    NVIC_EnableIRQ(STIMER_CMPR0_IRQn);
    NVIC_SetPriority(STIMER_CMPR0_IRQn, 2);
}

void fd_rtos_task_fallthrough(void) {
    while (true) {
    }
}

void fd_rtos_add_task(fd_rtos_entry_point_t entry_point, void *stack, size_t stack_size) {
    if (fd_rtos_task_count >= fd_rtos_task_size) {
        return;
    }
    memset(stack, 0, stack_size);
    fd_rtos_task_t *task = &fd_rtos_tasks[fd_rtos_task_index];
    task->sp = ((uint8_t *)stack) + stack_size;
    task->sp -= sizeof(fd_rtos_stack_frame_t);
    fd_rtos_stack_frame_t *stack_frame = task->sp;
    memset(stack_frame, 0, sizeof(stack_frame));
    stack_frame->hardware.lr = (uint32_t)fd_rtos_task_fallthrough;
    stack_frame->hardware.pc = (uint32_t)entry_point;
    stack_frame->hardware.psr = 0x21000000;
    task->runnable = true;
    ++fd_rtos_task_index;
}

// schedule exception handler

void fd_rtos_reschedule(void) __attribute__((naked));

void fd_rtos_reschedule(void) {
    // verify current task uses PSP
    uint32_t lr;
    __asm("MOV r0, lr" : "=r" (lr));
    fd_rtos_assert(lr & 0x0004);

    // suspend current task
    fd_rtos_task_t *task = &fd_rtos_tasks[fd_rtos_task_index];
    // push r4-r11 onto the program stack
    void *psp;
    __asm(
        "MRS r0, psp\n"
        "STMDB r0!, {r4-r11}\n"
        "MSR psp, r0\n"
        : "=r" (psp));
    task->sp = psp;

    // find new task to run
    do {
        if (++fd_rtos_task_index >= fd_rtos_task_count) {
            fd_rtos_task_index = 0;
        }
        task = &fd_rtos_tasks[fd_rtos_task_index];
    } while (task->runnable);

    // resume current task
    // pop r4-r11 from the program stack
    psp = task->sp;
    __asm(
        "LDMFD r0!, {r4-r11}\n"
        "MSR psp, r0\n"
        : : "r" (psp));

    // return from this exception handler
    const uint32_t EXC_RETURN_THREAD_PSP = 0xfffffffd;
    __asm("bx r0" : : "r"(EXC_RETURN_THREAD_PSP));
}

void fd_rtos_start(void) {
}

void fd_rtos_yield(void) {
    // invoke scheduler exception handler
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void STIMER_CMPR0_IRQHandler(void) {
    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREA);
    am_hal_stimer_int_disable(AM_HAL_STIMER_INT_COMPAREA);

    fd_rtos_task_t *task = &fd_rtos_tasks[fd_rtos_task_index];
    task->runnable = true;

    // invoke scheduler exception handler
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void fd_rtos_delay_delta(uint32_t delta) {
    fd_rtos_task_t *task = &fd_rtos_tasks[fd_rtos_task_index];
    task->runnable = false;
    am_hal_stimer_int_enable(AM_HAL_STIMER_INT_COMPAREA);
    uint32_t compare_a = 0;
    am_hal_stimer_compare_delta_set(compare_a, delta);

    // invoke scheduler exception handler
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

// 3 mHz stimer clock & round up
#define fd_rtos_delay(s) fd_rtos_delay_delta((uint32_t)((s) * 3000000.0) + 1)

uint32_t main_work_count = 0;

void main_work_task(void) {
    while (true) {
        uint32_t count = 0;
        for (int i = 0; i < 1000; ++i) {
            ++count;
        }
        ++main_work_count;
        fd_rtos_yield();
    }
}

uint32_t main_delay_count = 0;

void main_delay_task(void) {
    while (true) {
        ++main_delay_count;
        fd_rtos_delay(1.0);
    }
}

void fd_rtos_test(void) {
    fd_rtos_initialize();
    uint8_t work_stack[1024];
    fd_rtos_add_task(main_work_task, work_stack, sizeof(work_stack));
    uint8_t delay_stack[1024];
    fd_rtos_add_task(main_delay_task, delay_stack, sizeof(delay_stack));
//    fd_rtos_run();
}