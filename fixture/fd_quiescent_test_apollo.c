#include "fd_quiescent_test.h"

#include "mcu.h"

#include <stdbool.h>
#include <stdint.h>

void fd_quiescent_test(void) {
    while (true) {
        __WFI();
    }
}
