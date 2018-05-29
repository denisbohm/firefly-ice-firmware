#include "fd_quiescent_test.h"

#include <mcu.h>

static
void halt(void) {
    __asm("BKPT   #0");
}

void main(void) {
    void* tasks[] = {
        halt,

        SystemInit,

        fd_quiescent_test,
    };

    fd_quiescent_test();
}