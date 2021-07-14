#ifndef fd_graphics_h
#define fd_graphics_h

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    int x;
    int y;
} fd_graphics_point_t;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} fd_graphics_area_t;

extern const fd_graphics_area_t fd_graphics_area_empty;

bool fd_graphics_area_is_empty(fd_graphics_area_t area);
bool fd_graphics_area_contains_point(fd_graphics_area_t area, int x, int y);
bool fd_graphics_area_contains(fd_graphics_area_t a, fd_graphics_area_t b);
bool fd_graphics_area_is_equal(fd_graphics_area_t a, fd_graphics_area_t b);
bool fd_graphics_area_intersects(fd_graphics_area_t a, fd_graphics_area_t b);
fd_graphics_area_t fd_graphics_area_intersection(fd_graphics_area_t a, fd_graphics_area_t b);
fd_graphics_area_t fd_graphics_area_union(fd_graphics_area_t a, fd_graphics_area_t b);

// fd_graphics_bitmap_t
//
// for a 1-bit depth bitmap:
// MSB of first byte is x = 0, next byte is x = 8, etc
// first byte is y = 0, plus stride is y = 1, ...
// stride is 8 * ceil(width / 8)
//
// for an n-bit depth bitmap (where 1 <= n && n <= 8):
// bits for x = 0 start at MSB of first byte, followed by bits for x = 1, etc - packed
// first byte is y = 0, plus stride is y = 1, ...
// stride is 8 * ceil((width * depth) / 8)
//
// 2-bit depth is typically used for anti-aliased icons and font glyphs with only an alpha channel
//
// full color (16-bit R5-G6-B5 or 24-bit) is typically used for the frame buffer (and full color icons)

typedef struct {
    int width;
    int height;
    int depth;
    fd_graphics_point_t origin;
    const uint8_t *data;
} fd_graphics_bitmap_t;

int fd_graphics_bitmap_get_pixel(const fd_graphics_bitmap_t *bitmap, int x, int y);
fd_graphics_area_t fd_graphics_bitmap_get_bounds(const fd_graphics_bitmap_t *bitmap);

typedef struct {
    int width;
    int height;
    uint8_t *data;
} fd_graphics_image_t;

uint8_t *fd_graphics_image_get_pixel(const fd_graphics_image_t *image, int x, int y);

typedef struct {
    int character;
    int advance;
    fd_graphics_bitmap_t bitmap;
} fd_graphics_glyph_t;

typedef struct {
    const char *name;
    float height;
    int advance;
    int ascent;
    int glyph_count;
    const fd_graphics_glyph_t *glyphs;
} fd_graphics_font_t;

typedef struct {
    fd_graphics_area_t bounds;
    int advance;
} fd_graphics_metrics_t;

typedef union {
    uint32_t argb;
    struct {
        uint32_t b:8;
        uint32_t g:8;
        uint32_t r:8;
        uint32_t a:8;
    };
} fd_graphics_color_t;

typedef struct fd_graphics_s fd_graphics_t;

typedef struct {
    void (*write_background)(fd_graphics_t *graphics);
    void (*write_area)(fd_graphics_t *graphics, fd_graphics_area_t area);
    void (*write_image)(fd_graphics_t *graphics, int x, int y, const fd_graphics_image_t *image);
    void (*write_bitmap)(fd_graphics_t *graphics, int x, int y, const fd_graphics_bitmap_t *bitmap);
} fd_graphics_backend_t;

struct fd_graphics_s {
    int width;
    int height;
    fd_graphics_backend_t backend;
    uint8_t *buffer;
    fd_graphics_area_t clipping;
    const fd_graphics_font_t *font;
    fd_graphics_color_t foreground;
    fd_graphics_color_t background;
};

void fd_graphics_ram_initialize(fd_graphics_t *graphics, int width, int height, uint8_t *buffer);

void fd_graphics_initialize(fd_graphics_t *graphics, int width, int height, fd_graphics_backend_t backend, uint8_t *buffer);
void fd_graphics_reset(fd_graphics_t *graphics);

void fd_graphics_set_clipping(fd_graphics_t *graphics, fd_graphics_area_t area);
void fd_graphics_remove_clipping(fd_graphics_t *graphics);
fd_graphics_area_t aw_graphics_get_clipping(fd_graphics_t *graphics);

void fd_graphics_set_font(fd_graphics_t *graphics, const fd_graphics_font_t *font);
void fd_graphics_set_background(fd_graphics_t *graphics, fd_graphics_color_t color);
void fd_graphics_set_foreground(fd_graphics_t *graphics, fd_graphics_color_t color);

fd_graphics_metrics_t fd_graphics_get_string_metrics_length(fd_graphics_t *graphics, const char *string, size_t length);
fd_graphics_metrics_t fd_graphics_get_string_metrics(fd_graphics_t *graphics, const char *string);

void fd_graphics_write_background(fd_graphics_t *graphics);
void fd_graphics_write_area(fd_graphics_t *graphics, fd_graphics_area_t area);
void fd_graphics_write_image(fd_graphics_t *graphics, int x, int y, const fd_graphics_image_t *image);
void fd_graphics_write_bitmap(fd_graphics_t *graphics, int x, int y, const fd_graphics_bitmap_t *bitmap);
void fd_graphics_write_string_length(fd_graphics_t *graphics, int x, int y, const char *string, size_t length);
void fd_graphics_write_string(fd_graphics_t *graphics, int x, int y, const char *string);

#endif
