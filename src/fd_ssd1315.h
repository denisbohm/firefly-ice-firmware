#ifndef fd_ssd1315_h
#define fd_ssd1315_h

#include "fd_i2cm.h"

#include <stdint.h>

void fd_ssd1315_initialize(const fd_i2cm_device_t *device);

void fd_ssd1315_display_on(void);
void fd_ssd1315_display_off(void);

void fd_ssd1315_write_image_start(int x, int y, int width, int height);
void fd_ssd1315_write_image_subdata(const uint8_t *data, int length);
void fd_ssd1315_write_image_end(void);

#endif
