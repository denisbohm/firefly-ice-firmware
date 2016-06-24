#ifndef FDI_DETOUR_H
#define FDI_DETOUR_H

#include "fd_detour.h"

void fdi_detour_source_initialize(fd_detour_t *source, uint8_t *data, uint32_t size);
bool fdi_detour_source_is_transferring(fd_detour_t *source);
void fdi_detour_source_set(fd_detour_t *source, uint32_t length);
bool fdi_detour_source_get(fd_detour_t *source, uint8_t *data, uint32_t length);

#endif
