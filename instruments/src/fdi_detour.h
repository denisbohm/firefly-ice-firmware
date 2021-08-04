#ifndef fdi_detour_h
#define fdi_detour_h

/*
A "detour" is used to send arbitrary data via some other transport, such as USB or Bluetooth Low Energy.
Each of these other transports has small limits on the size of data that can be sent in one chunk.  The
detour can split and reassemble the arbitrary data into chunks that can be sent over the transport.

The first byte of each chunk is a sequence number, with the first chunk indicated by the sequence number 0.
In the first chunk the sequence number is followed by a uint16 length.  All subsequent data in chunks is
the content data.  The last chunk can have extra data (which will be ignored).
*/

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    fdi_detour_state_clear,
    fdi_detour_state_intermediate,
    fdi_detour_state_success,
    fdi_detour_state_error
} fdi_detour_state_t;

typedef struct {
    uint8_t *data;
    uint32_t size;

    fdi_detour_state_t state;
    uint32_t length;
    uint32_t sequence_number;
    uint32_t offset;
} fdi_detour_t;

void fdi_detour_initialize(fdi_detour_t *detour, uint8_t *data, uint32_t size);

void fdi_detour_clear(fdi_detour_t *detour);

fdi_detour_state_t fdi_detour_state(fdi_detour_t *detour);

void fdi_detour_event(fdi_detour_t *detour, uint8_t *data, uint32_t length);

#endif
