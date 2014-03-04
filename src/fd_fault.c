#include "fd_log.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* The prototype shows it is a naked function - in effect this is just an assembly function. */
void HardFault_Handler(void) __attribute__((naked));

// This fault handler implementation calls fdFaultGetRegistersFromStack
void HardFault_Handler(void) {
    __asm volatile(
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word fdFaultGetRegistersFromStack    \n"
    );
}

void fdFaultGetRegistersFromStack(uint32_t *pulFaultStackAddress) __attribute__((used));

void fdFaultGetRegistersFromStack(uint32_t *pulFaultStackAddress) {
    /* These are volatile to try and prevent the compiler/linker optimizing them
    away as the variables never actually get used.  If the debugger won't show the
    values of the variables, make them global my moving their declaration outside
    of this function. */
    volatile uint32_t  __attribute__((unused))r0;
    volatile uint32_t  __attribute__((unused))r1;
    volatile uint32_t  __attribute__((unused))r2;
    volatile uint32_t  __attribute__((unused))r3;
    volatile uint32_t  __attribute__((unused))r12;
    volatile uint32_t lr;
    volatile uint32_t pc;
    volatile uint32_t  __attribute__((unused))psr;

    r0 = pulFaultStackAddress[0];
    r1 = pulFaultStackAddress[1];
    r2 = pulFaultStackAddress[2];
    r3 = pulFaultStackAddress[3];

    r12 = pulFaultStackAddress[4];
    lr = pulFaultStackAddress[5];
    pc = pulFaultStackAddress[6];
    psr = pulFaultStackAddress[7];

    // Configurable Fault Status Register
    // Consists of MMSR, BFSR and UFSR
    uint32_t __attribute__((unused)) CFSR = (*((volatile unsigned long *)(0xE000ED28))) ;

    // Hard Fault Status Register
    uint32_t __attribute__((unused)) HFSR = (*((volatile unsigned long *)(0xE000ED2C))) ;

    // Debug Fault Status Register
    uint32_t __attribute__((unused)) DFSR = (*((volatile unsigned long *)(0xE000ED30))) ;

    // Auxiliary Fault Status Register
    uint32_t __attribute__((unused)) AFSR = (*((volatile unsigned long *)(0xE000ED3C))) ;

    // Read the Fault Address Registers. These may not contain valid values.
    // Check BFARVALID/MMARVALID to see if they are valid values
    // MemManage Fault Address Register
    uint32_t __attribute__((unused)) MMAR = (*((volatile unsigned long *)(0xE000ED34))) ;
    // Bus Fault Address Register
    uint32_t __attribute__((unused)) BFAR = (*((volatile unsigned long *)(0xE000ED38))) ;

    char buffer[80];
    sprintf(buffer, "pc=%x lr=%x", (unsigned int)pc, (unsigned int)lr);
    fd_log_assert_fail(buffer);

    /* When the following line is hit, the variables contain the register values. */
    while (true);
}