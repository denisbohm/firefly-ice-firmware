#ifndef FDI_APOLLO2_H
#define FDI_APOLLO2_H

#include "fdi_serial_wire_debug.h"

bool fdi_apollo2_flash_mass_erase_from_sram(fdi_serial_wire_t *serial_wire, uint32_t instance, fdi_serial_wire_debug_error_t *error);

bool fdi_apollo2_flash_program_main_from_sram(fdi_serial_wire_t *serial_wire, uint32_t address, uint8_t *data, uint32_t length, fdi_serial_wire_debug_error_t *error);

bool fdi_apollo2_flash_info_erase_from_sram(fdi_serial_wire_t *serial_wire, fdi_serial_wire_debug_error_t *error);

bool fdi_apollo2_flash_program_info_area_from_sram(fdi_serial_wire_t *serial_wire, uint32_t offset, uint8_t *data, uint32_t length, fdi_serial_wire_debug_error_t *error);

bool fdi_apollo2_test(fdi_serial_wire_t *serial_wire);

#endif