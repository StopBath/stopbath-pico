/*
 * The frame buffer, in the packing the panel is sent.
 *
 * No SDK dependency, so every pixel the firmware will show can be asserted
 * on the development machine. The packing is the manufacturer's for this
 * panel (evaluation log 4.3): one bit per pixel, eight horizontal pixels per
 * byte with the leftmost in the most significant bit, rows in order, 1 for
 * white and 0 for black. The buffer is exactly what the vendored driver's
 * display call takes, so there is no conversion step to get wrong.
 *
 * Every drawing operation clips to the panel. Nothing here allocates: the
 * buffer is the caller's, sized once at start (Flipper 0.10).
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REMOTE_BITMAP_WIDTH 400
#define REMOTE_BITMAP_HEIGHT 300
#define REMOTE_BITMAP_BYTES_PER_ROW (REMOTE_BITMAP_WIDTH / 8)
#define REMOTE_BITMAP_SIZE_BYTES (REMOTE_BITMAP_BYTES_PER_ROW * REMOTE_BITMAP_HEIGHT)

typedef struct {
    uint8_t bytes[REMOTE_BITMAP_SIZE_BYTES];
} RemoteBitmap;

typedef enum {
    RemoteBitmapWhite,
    RemoteBitmapBlack,
} RemoteBitmapColour;

void remote_bitmap_clear(RemoteBitmap* bitmap, RemoteBitmapColour colour);

/* A pixel outside the panel is ignored. */
void remote_bitmap_set_pixel(RemoteBitmap* bitmap, int x, int y, RemoteBitmapColour colour);

/* A pixel outside the panel reads white, the panel's background. */
RemoteBitmapColour remote_bitmap_get_pixel(const RemoteBitmap* bitmap, int x, int y);

/* Fills width by height pixels from (x, y), clipped to the panel. A zero or
 * negative width or height draws nothing. */
void remote_bitmap_fill_rectangle(
    RemoteBitmap* bitmap,
    int x,
    int y,
    int width,
    int height,
    RemoteBitmapColour colour);

#ifdef __cplusplus
}
#endif
