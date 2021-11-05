#include "fd_graphics_ssd1315.h"

#include "fd_graphics.h"
#include "fd_ssd1315.h"

#include <string.h>

typedef struct {
    fd_graphics_t graphics;
    uint8_t buffer[320 * 3];
} fd_graphics_ssd1315_t;

static fd_graphics_ssd1315_t fd_graphics_ssd1315;

static void fd_graphics_ssd1315_write_background(fd_graphics_t *graphics) {
    fd_ssd1315_write_image_start(0, 0, 240, 320);

    uint8_t *data = graphics->buffer;
    for (int cy = 0; cy < 320; ++cy) {
        *data++ = graphics->background.r;
        *data++ = graphics->background.g;
        *data++ = graphics->background.b;
    }
    for (int cx = 0; cx < 240; ++cx) {
        fd_ssd1315_write_image_subdata(graphics->buffer, 320 * 3);
    }

    fd_ssd1315_write_image_end();
}

static void fd_graphics_ssd1315_image_start(int x, int y, int width, int height) {
    fd_ssd1315_write_image_start(x, y, width, height);
}

static void fd_graphics_ssd1315_write_area(fd_graphics_t *graphics, fd_graphics_area_t unclipped_area) {
    fd_graphics_area_t area = fd_graphics_area_intersection(unclipped_area, graphics->clipping);
    fd_graphics_ssd1315_image_start(area.x, area.y, area.width, area.height);

    uint8_t *data = graphics->buffer;
    for (int cy = 0; cy < area.height; ++cy) {
        *data++ = graphics->foreground.r;
        *data++ = graphics->foreground.g;
        *data++ = graphics->foreground.b;
    }
    for (int cx = 0; cx < area.width; ++cx) {
        fd_ssd1315_write_image_subdata(graphics->buffer, area.height * 3);
    }

    fd_ssd1315_write_image_end();
}

static void fd_graphics_ssd1315_write_image(fd_graphics_t *graphics, int x, int y, const fd_graphics_image_t *image) {
    fd_graphics_area_t unclipped_dst_area = { .x = x, .y = y, .width = image->width, .height = image->height };
    fd_graphics_area_t dst_area = fd_graphics_area_intersection(unclipped_dst_area, graphics->clipping);
    fd_graphics_ssd1315_image_start(dst_area.x, dst_area.y, dst_area.width, dst_area.height);

    int sx = dst_area.x - x;
    int sy = dst_area.y - y;
    for (int cx = 0; cx < dst_area.width; ++cx) {
        uint8_t *data = graphics->buffer;
        uint8_t *src = fd_graphics_image_get_pixel(image, sx + cx, sy);
        for (int cy = 0; cy < dst_area.height; ++cy) {
            *data++ = *src++;
            *data++ = *src++;
            *data++ = *src++;
        }
        fd_ssd1315_write_image_subdata(graphics->buffer, dst_area.height * 3);
    }

    fd_ssd1315_write_image_end();
}

static void fd_graphics_ssd1315_write_bitmap(fd_graphics_t *graphics, int rx, int ry, const fd_graphics_bitmap_t *bitmap) {
    int x = rx - bitmap->origin.x;
    int y = ry - bitmap->origin.y;
    fd_graphics_area_t unclipped_dst_area = { .x = x, .y = y, .width = bitmap->width, .height = bitmap->height };
    fd_graphics_area_t dst_area = fd_graphics_area_intersection(unclipped_dst_area, graphics->clipping);
    fd_graphics_ssd1315_image_start(dst_area.x, dst_area.y, dst_area.width, dst_area.height);

    int sx = dst_area.x - x;
    int sy = dst_area.y - y;
    int fr = graphics->foreground.r;
    int fg = graphics->foreground.g;
    int fb = graphics->foreground.b;
    int sr = graphics->background.r;
    int sg = graphics->background.g;
    int sb = graphics->background.b;
    int max = (1 << bitmap->depth) - 1;
    for (int cx = 0; cx < dst_area.width; ++cx) {
        uint8_t *data = graphics->buffer;
        for (int cy = 0; cy < dst_area.height; ++cy) {
            int alpha = fd_graphics_bitmap_get_pixel(bitmap, sx + cx, sy + cy);
            *data++ = (uint8_t)((fr * alpha + sr * (max - alpha)) / max);
            *data++ = (uint8_t)((fg * alpha + sg * (max - alpha)) / max);
            *data++ = (uint8_t)((fb * alpha + sb * (max - alpha)) / max);
        }
        fd_ssd1315_write_image_subdata(graphics->buffer, bitmap->height * 3);
    }

    fd_ssd1315_write_image_end();
}

fd_graphics_t *fd_graphics_ssd1315_get(void) {
    return &fd_graphics_ssd1315.graphics;
}

void fd_graphics_ssd1315_initialize(void) {
    memset(&fd_graphics_ssd1315, 0, sizeof(fd_graphics_ssd1315));

    fd_ssd1315_initialize(0);
    fd_ssd1315_display_on();

    fd_graphics_backend_t backend = {
        .write_background = fd_graphics_ssd1315_write_background,
        .write_area = fd_graphics_ssd1315_write_area,
        .write_image = fd_graphics_ssd1315_write_image,
        .write_bitmap = fd_graphics_ssd1315_write_bitmap,
    };
    fd_graphics_initialize(&fd_graphics_ssd1315.graphics, 240, 320, backend, fd_graphics_ssd1315.buffer);
}
