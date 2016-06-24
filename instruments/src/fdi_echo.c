#include "fdi_echo.h"

#include "fdi_api.h"

#define FDI_ECHO_TYPE 0x00000001

void fdi_echo_function(fd_binary_t *binary) {
    uint8_t *data = &binary->buffer[binary->get_index];
    uint32_t length = binary->size - binary->get_index;
    fdi_api_send(FDI_ECHO_TYPE, data, length);
}

void fdi_echo_initialize(void) {
   fdi_api_register(FDI_ECHO_TYPE, fdi_echo_function);
}
