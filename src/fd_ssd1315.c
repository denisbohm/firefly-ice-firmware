#include "fd_ssd1315.h"

#include "fd_delay.h"
#include "fd_gpio.h"
#include "fd_i2cm.h"
#include "fd_log.h"
#include "fd_unused.h"

#include <stddef.h>

// i2c address 0x78

typedef struct {
    const fd_i2cm_device_t *device;
} fd_ssd1315_t;

fd_ssd1315_t fd_ssd1315;

static void fd_ssd1315_write(const uint8_t *data, size_t size) {
    fd_i2cm_transfer_t transfers[] = {
        {
            .direction = fd_i2cm_direction_tx,
            .bytes = (uint8_t *)data,
            .byte_count = size,
        },
    };
    fd_i2cm_io_t io = {
        .transfers = transfers,
        .transfer_count = sizeof(transfers) / sizeof(transfers[0]),
    };
    fd_i2cm_device_io(fd_ssd1315.device, &io);
}

static void fd_ssd1315_write_command(uint8_t command) {
    uint8_t data[] = { 0x00, command };
    fd_ssd1315_write(data, sizeof(data));
}

static void fd_ssd1315_write_command_arg1(uint8_t command, uint8_t arg1) {
    uint8_t data[] = { 0x00, command, arg1 };
    fd_ssd1315_write(data, sizeof(data));
}

static void fd_ssd1315_write_command_arg2(uint8_t command, uint8_t arg1, uint8_t arg2) {
    uint8_t data[] = { 0x00, command, arg1, arg2 };
    fd_ssd1315_write(data, sizeof(data));
}

static void fd_ssd1315_send_init_sequence(void) {
    fd_ssd1315_write_command(0xae); // display off
    fd_ssd1315_write_command(0x40); // set display start line
    fd_ssd1315_write_command_arg1(0x20, 0x00); // set addressing mode: horizontal
    fd_ssd1315_write_command_arg2(0x21, 0, 127); // set column address
    fd_ssd1315_write_command_arg2(0x22, 0, 7); // set page address
    fd_ssd1315_write_command_arg1(0x81, 0x45); // set contrast

    fd_ssd1315_write_command_arg1(0xa8, 0x3f); // set multiplex ratio

    fd_ssd1315_write_command(0xa1); // set segment remap: columns address 127 is mapped to SEG0
    fd_ssd1315_write_command(0xc8); // set COM output scan direction: COM0 to COM[N-1]

    fd_ssd1315_write_command_arg1(0xd3, 0x00); // set display offset: 0
    fd_ssd1315_write_command_arg1(0xd5, 0x80); // set display clock divide ratio (0x00=1) and oscillator frequency (0x8)
    fd_ssd1315_write_command_arg1(0xd9, 0x22); // set pre-charge period: 0x022/f1
    fd_ssd1315_write_command_arg1(0xda, 0x12); // set com pins HW config, sequential com pin config (bit 4), disable left/right remap (bit 5)
    fd_ssd1315_write_command_arg1(0xdb, 0x20); // set vcomh deselect level
    fd_ssd1315_write_command_arg1(0x8d, 0x15); // charge pump setting

    fd_ssd1315_write_command(0x2e); // Deactivate scroll
    fd_ssd1315_write_command(0xa4); // RAM content display
    fd_ssd1315_write_command(0xa6); // normal display (not inverted)
}

void fd_ssd1315_display_on(void) {
    fd_ssd1315_write_command(0xaf);
//    fd_ssd1315_write_command(0xa5); // entire display on
}

void fd_ssd1315_display_off(void) {
    fd_ssd1315_write_command(0xae);
}

void fd_ssd1315_initialize(const fd_i2cm_device_t *device) {
    fd_ssd1315.device = device;
    fd_ssd1315_send_init_sequence();
}

void fd_ssd1315_write_image_start(int x, int y, int width, int height) {
    fd_log_assert((0 <= x) && (x <= 127));
    fd_log_assert((width > 0) && (0 <= (x + width)) && ((x + width) <= 128));
    fd_log_assert((0 <= y) && (y <= 63));
    fd_log_assert((height > 0) && (0 <= (y + height)) && ((y + height) <= 64));
    fd_log_assert((y & 0x7) == 0);
    fd_log_assert((height & 0x7) == 0);

    fd_ssd1315_write_command_arg2(0x21, x, x + width - 1);
    fd_ssd1315_write_command_arg2(0x22, y / 8, (y + height) / 8 - 1);
}

void fd_ssd1315_write_image_subdata(const uint8_t *data, int length) {
    uint8_t byte = 0x40;
    fd_i2cm_transfer_t transfers[] = {
        {
            .direction = fd_i2cm_direction_tx,
            .bytes = (uint8_t *)&byte,
            .byte_count = 1,
        },
        {
            .direction = fd_i2cm_direction_tx,
            .bytes = (uint8_t *)data,
            .byte_count = length,
        },
    };
    fd_i2cm_io_t io = {
        .transfers = transfers,
        .transfer_count = sizeof(transfers) / sizeof(transfers[0]),
    };
    fd_i2cm_device_io(fd_ssd1315.device, &io);
}

void fd_ssd1315_write_image_end(void) {
}
