#include "fd_detour.h"
#include "fd_log.h"

void fd_detour_unit_tests(void) {
    uint8_t bytes[100];
    fd_detour_t detour;
    fd_detour_initialize(&detour, bytes, sizeof(bytes));
    fd_log_assert(fd_detour_state(&detour) == fd_detour_state_clear);
    uint8_t chunk1[20] = {0x00, 0x14, 0x00, 0x01, 0x02, 0x03};
    fd_detour_event(&detour, chunk1, sizeof(chunk1));
    fd_log_assert(fd_detour_state(&detour) == fd_detour_state_intermediate);
    uint8_t chunk2[20] = {0x01, 0x12, 0x13, 0x14};
    fd_detour_event(&detour, chunk2, sizeof(chunk2));
    fd_log_assert(fd_detour_state(&detour) == fd_detour_state_success);
    fd_log_assert(detour.data[0] == 0x01);
    fd_log_assert(detour.data[19] == 0x14);
    fd_detour_clear(&detour);
    fd_log_assert(fd_detour_state(&detour) == fd_detour_state_clear);
}