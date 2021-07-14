#include "fd_graphics.h"

#include "fd_log.h"

#include <math.h>
#include <string.h>

static int MIN(int a, int b) {
    return a < b ? a : b;
}

static int MAX(int a, int b) {
    return a > b ? a : b;
}

const fd_graphics_area_t fd_graphics_area_empty = { .x = 0, .y = 0, .width = 0, .height = 0 };

bool fd_graphics_area_is_empty(fd_graphics_area_t a) {
    return (a.width == 0) || (a.height == 0);
}

bool fd_graphics_area_is_equal(fd_graphics_area_t a, fd_graphics_area_t b) {
    return (a.x == b.x) || (a.y == b.y) || (a.width == b.width) || (a.height == b.height);
}

bool fd_graphics_area_contains_point(fd_graphics_area_t area, int x, int y) {
    return (area.x <= x) && (x < (area.x + area.width)) && (area.y <= y) && (y < (area.y + area.height));
}

bool fd_graphics_area_contains(fd_graphics_area_t a, fd_graphics_area_t b) {
    return (b.x >= a.x) && (b.y >= a.y) && ((b.x + b.width) <= (a.x + a.width)) && ((b.y + b.height) <= (a.y + a.height));
}

fd_graphics_area_t fd_graphics_area_intersection(fd_graphics_area_t a, fd_graphics_area_t b) {
    int x = MAX(a.x, b.x);
    int x2 = MIN(a.x + a.width, b.x + b.width);
    int y = MAX(a.y, b.y);
    int y2 = MIN(a.y + a.height, b.y + b.height);
    if ((x2 >= x) && (y2 >= y)) {
        fd_graphics_area_t area = {
            .x = x,
            .y = y,
            .width = x2 - x,
            .height = y2 - y,
        };
        return area;
    }
    return fd_graphics_area_empty;
}

bool fd_graphics_area_intersects(fd_graphics_area_t a, fd_graphics_area_t b) {
    return !fd_graphics_area_is_empty(fd_graphics_area_intersection(a, b));
}

fd_graphics_area_t fd_graphics_area_union(fd_graphics_area_t a, fd_graphics_area_t b) {
    if (fd_graphics_area_is_empty(a)) {
        return b;
    }
    if (fd_graphics_area_is_empty(b)) {
        return a;
    }
    int min_x = MIN(a.x, b.x);
    int a_max_x = a.x + a.width;
    int b_max_x = b.x + b.width;
    int max_x = MAX(a_max_x, b_max_x);
    int min_y = MIN(a.y, b.y);
    int a_max_y = a.y + a.height;
    int b_max_y = b.y + b.height;
    int max_y = MAX(a_max_y, b_max_y);
    return (fd_graphics_area_t) {
        .x = min_x,
        .y = min_y,
        .width = max_x - min_x,
        .height = max_y - min_y
    };
}

uint8_t *fd_graphics_image_get_pixel(const fd_graphics_image_t *image, int x, int y) {
    return &image->data[(x * image->height + y) * 3];
}

static const fd_graphics_font_t fd_graphics_empty_font = {
    .name = "empty",
    .height = 0,
    .glyph_count = 0,
    .glyphs = 0,
};

int fd_graphics_bitmap_get_pixel(const fd_graphics_bitmap_t *bitmap, int x, int y) {
    int stride = (bitmap->width * bitmap->depth + 7) / 8;
    int mask = (0b1111111100000000 >> bitmap->depth) & 0b11111111;

    int index = y * stride + (x * bitmap->depth) / 8;
    int shift = (x * bitmap->depth) % 8;
    int value = (bitmap->data[index] & (mask >> shift)) >> (8 - bitmap->depth - shift);
    return value;
}

const fd_graphics_glyph_t *fd_graphics_font_get_glyph(const fd_graphics_font_t *font, int character) {
    for (int i = 0; i < font->glyph_count; ++i) {
        const fd_graphics_glyph_t *glyph = &font->glyphs[i];
        if (glyph->character == character) {
            return glyph;
        }
    }
    return 0;
}

void fd_graphics_ram_write_background(fd_graphics_t *graphics) {
    uint8_t *data = graphics->buffer;
    for (int cx = 0; cx < graphics->width; ++cx) {
        for (int cy = 0; cy < graphics->height; ++cy) {
            *data++ = graphics->background.r;
            *data++ = graphics->background.g;
            *data++ = graphics->background.b;
        }
    }
}

uint8_t *fd_graphics_ram_get_pixel(const fd_graphics_t *graphics, int x, int y) {
    return &graphics->buffer[(x * graphics->height + y) * 3];
}

void fd_graphics_ram_write_area(fd_graphics_t *graphics, fd_graphics_area_t unclipped_area) {
    fd_graphics_area_t area = fd_graphics_area_intersection(unclipped_area, graphics->clipping);
    uint32_t src = graphics->foreground.argb;
    uint32_t sa = (src >> 24) & 0xff;
    uint32_t sr = (src >> 16) & 0xff;
    uint32_t sg = (src >> 8) & 0xff;
    uint32_t sb = src & 0xff;
    uint32_t da = 0xff - sa;
    int x = area.x;
    int y = area.y;
    int x_end = x + area.width;
    int y_end = y + area.height;
    for (int cx = x; cx < x_end; ++cx) {
        uint8_t *dst = fd_graphics_ram_get_pixel(graphics, cx, y);
        for (int cy = y; cy < y_end; ++cy) {
            *dst = (uint8_t)((sr * sa + *dst * da) >> 8);
            ++dst;
            *dst = (uint8_t)((sg * sa + *dst * da) >> 8);
            ++dst;
            *dst = (uint8_t)((sb * sa + *dst * da) >> 8);
            ++dst;
        }
    }
}

void fd_graphics_ram_write_image(fd_graphics_t *graphics, int x, int y, const fd_graphics_image_t *image) {
    fd_graphics_area_t unclipped_dst_area = { .x = x, .y = y, .width = image->width, .height = image->height };
    fd_graphics_area_t dst_area = fd_graphics_area_intersection(unclipped_dst_area, graphics->clipping);
    int sx = dst_area.x - x;
    int sy = dst_area.y - y;
    int dx = x + sx;
    int dy = y + sy;
    for (int cx = 0; cx < dst_area.width; ++cx) {
        uint8_t *src = fd_graphics_image_get_pixel(image, sx + cx, sy);
        uint8_t *dst = fd_graphics_ram_get_pixel(graphics, dx + cx, dy);
        for (int cy = 0; cy < dst_area.height; ++cy) {
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src++;
        }
    }
}

void fd_graphics_ram_write_bitmap(fd_graphics_t *graphics, int rx, int ry, const fd_graphics_bitmap_t *bitmap) {
    int x = rx - bitmap->origin.x;
    int y = ry - bitmap->origin.y;
    fd_graphics_area_t unclipped_dst_area = { .x = x, .y = y, .width = bitmap->width, .height = bitmap->height };
    fd_graphics_area_t dst_area = fd_graphics_area_intersection(unclipped_dst_area, graphics->clipping);
    int sx = dst_area.x - x;
    int sy = dst_area.y - y;
    int dx = x + sx;
    int dy = y + sy;
    uint32_t src = graphics->foreground.argb;
    int sr = (src >> 16) & 0xff;
    int sg = (src >> 8) & 0xff;
    int sb = src & 0xff;
    int max = (1 << bitmap->depth) - 1;
    float scale = 255.0f / (float)max;
    for (int cx = 0; cx < dst_area.width; ++cx) {
        uint8_t *dst = fd_graphics_ram_get_pixel(graphics, dx + cx, dy);
        for (int cy = 0; cy < dst_area.height; ++cy) {
            int a = fd_graphics_bitmap_get_pixel(bitmap, sx + cx, sy + cy);
            int sa = (int)(scale * (float)a + 0.5f);
            int da = 0xff - sa;
            *dst = (uint8_t)((sr * sa + *dst * da) >> 8);
            ++dst;
            *dst = (uint8_t)((sg * sa + *dst * da) >> 8);
            ++dst;
            *dst = (uint8_t)((sb * sa + *dst * da) >> 8);
            ++dst;
        }
    }
}

void fd_graphics_ram_initialize(fd_graphics_t *graphics, int width, int height, uint8_t *buffer) {
    fd_graphics_backend_t backend = {
        .write_background = fd_graphics_ram_write_background,
        .write_area = fd_graphics_ram_write_area,
        .write_image = fd_graphics_ram_write_image,
        .write_bitmap = fd_graphics_ram_write_bitmap,
    };
    fd_graphics_initialize(graphics, width, height, backend, buffer);
}

void fd_graphics_initialize(fd_graphics_t *graphics, int width, int height, fd_graphics_backend_t backend, uint8_t *buffer) {
    graphics->width = width;
    graphics->height = height;
    graphics->backend = backend;
    graphics->buffer = buffer;
    fd_graphics_reset(graphics);
}

void fd_graphics_reset(fd_graphics_t *graphics) {
    graphics->clipping = (fd_graphics_area_t) {
        .x = 0,
        .y = 0,
        .width = graphics->width,
        .height = graphics->height,
    };
    graphics->font = &fd_graphics_empty_font;
    graphics->background = (fd_graphics_color_t) { .argb = 0xff000000 };
    graphics->foreground = (fd_graphics_color_t) { .argb = 0xffffffff };
 }

void fd_graphics_set_clipping(fd_graphics_t *graphics, fd_graphics_area_t area) {
    fd_graphics_area_t graphics_area = {
        .x = 0,
        .y = 0,
        .width = graphics->width,
        .height = graphics->height,
    };
    graphics->clipping = fd_graphics_area_intersection(graphics_area, area);
}

void fd_graphics_remove_clipping(fd_graphics_t *graphics) {
    graphics->clipping = (fd_graphics_area_t) {
        .x = 0,
        .y = 0,
        .width = graphics->width,
        .height = graphics->height,
    };
}

fd_graphics_area_t fd_graphics_get_clipping(fd_graphics_t *graphics) {
    return graphics->clipping;
}

void fd_graphics_set_font(fd_graphics_t *graphics, const fd_graphics_font_t *font) {
    graphics->font = font;
}

void fd_graphics_set_background(fd_graphics_t *graphics, fd_graphics_color_t color) {
    graphics->background = color;
}

void fd_graphics_set_foreground(fd_graphics_t *graphics, fd_graphics_color_t color) {
    graphics->foreground = color;
}

void fd_graphics_write_background(fd_graphics_t *graphics) {
    graphics->backend.write_background(graphics);
}

void fd_graphics_write_area(fd_graphics_t *graphics, fd_graphics_area_t area) {
    if ((area.x < 0) || (area.y < 0) || ((area.x + area.width) > graphics->width) || ((area.y + area.height) > graphics->height)) {
        fd_log_assert_fail("area off screen");
        return;
    }
    
    graphics->backend.write_area(graphics, area);
}

void fd_graphics_write_image(fd_graphics_t *graphics, int x, int y, const fd_graphics_image_t *image) {
    if ((x < 0) || (y < 0) || ((x + image->width) > graphics->width) || ((y + image->height) > graphics->height)) {
        fd_log_assert_fail("image off screen");
        return;
    }
    
    graphics->backend.write_image(graphics, x, y, image);
}

void fd_graphics_write_bitmap(fd_graphics_t *graphics, int x, int y, const fd_graphics_bitmap_t *bitmap) {
    int ox = x - bitmap->origin.x;
    int oy = y - bitmap->origin.y;
    if ((ox < 0) || (oy < 0) || ((ox + bitmap->width) > graphics->width) || ((oy + bitmap->height) > graphics->height)) {
        fd_log_assert_fail("bitmap off screen");
        return;
    }
    
    graphics->backend.write_bitmap(graphics, x, y, bitmap);
}

void fd_graphics_write_string_length(fd_graphics_t *graphics, int x, int y, const char *string, size_t length) {
    int cx = x;
    for (size_t i = 0; i < length; ++i) {
        int character = string[i] & 0xff;
        const fd_graphics_glyph_t *glyph = fd_graphics_font_get_glyph(graphics->font, character);
        if (glyph == 0) {
            cx += graphics->font->advance;
        } else {
            fd_graphics_write_bitmap(graphics, cx, y, &glyph->bitmap);
            cx += glyph->advance;
        }
    }
}

void fd_graphics_write_string(fd_graphics_t *graphics, int x, int y, const char *string) {
    fd_graphics_write_string_length(graphics, x, y, string, strlen(string));
}

fd_graphics_area_t fd_graphics_bitmap_get_bounds(const fd_graphics_bitmap_t *bitmap) {
    return (fd_graphics_area_t) {
        .x = -bitmap->origin.x,
        .y = -bitmap->origin.y,
        .width = bitmap->width,
        .height = bitmap->height,
    };
}

fd_graphics_metrics_t fd_graphics_get_string_metrics_length(fd_graphics_t *graphics, const char *string, size_t length) {
    fd_graphics_area_t bounds = { .x = 0, .y = 0, .width = 0, .height = 0 };
    int advance = 0;
    for (size_t i = 0; i < length; ++i) {
        int character = string[i] & 0xff;
        const fd_graphics_glyph_t *glyph = fd_graphics_font_get_glyph(graphics->font, character);
        if (glyph == 0) {
            advance += graphics->font->advance;
            continue;
        }
        fd_graphics_area_t bitmap_bounds = fd_graphics_bitmap_get_bounds(&glyph->bitmap);
        bitmap_bounds.x += advance;
        bounds = fd_graphics_area_union(bounds, bitmap_bounds);
        advance += glyph->advance;
    }
    return (fd_graphics_metrics_t) { .bounds = bounds, .advance = advance };
}

fd_graphics_metrics_t fd_graphics_get_string_metrics(fd_graphics_t *graphics, const char *string) {
    return fd_graphics_get_string_metrics_length(graphics, string, strlen(string));
}
