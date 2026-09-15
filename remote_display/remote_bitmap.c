#include "remote_bitmap.h"

#include <string.h>

static bool pixel_is_on_panel(int x, int y) {
    return x >= 0 && x < REMOTE_BITMAP_WIDTH && y >= 0 && y < REMOTE_BITMAP_HEIGHT;
}

void remote_bitmap_clear(RemoteBitmap* bitmap, RemoteBitmapColour colour) {
    memset(bitmap->bytes, colour == RemoteBitmapWhite ? 0xFF : 0x00, REMOTE_BITMAP_SIZE_BYTES);
}

void remote_bitmap_set_pixel(RemoteBitmap* bitmap, int x, int y, RemoteBitmapColour colour) {
    if(!pixel_is_on_panel(x, y)) {
        return;
    }
    size_t byte_index = (size_t)y * REMOTE_BITMAP_BYTES_PER_ROW + (size_t)(x / 8);
    /* Leftmost pixel in the most significant bit. */
    uint8_t mask = (uint8_t)(0x80u >> (x % 8));
    if(colour == RemoteBitmapWhite) {
        bitmap->bytes[byte_index] |= mask;
    } else {
        bitmap->bytes[byte_index] &= (uint8_t)~mask;
    }
}

RemoteBitmapColour remote_bitmap_get_pixel(const RemoteBitmap* bitmap, int x, int y) {
    if(!pixel_is_on_panel(x, y)) {
        return RemoteBitmapWhite;
    }
    size_t byte_index = (size_t)y * REMOTE_BITMAP_BYTES_PER_ROW + (size_t)(x / 8);
    uint8_t mask = (uint8_t)(0x80u >> (x % 8));
    return (bitmap->bytes[byte_index] & mask) ? RemoteBitmapWhite : RemoteBitmapBlack;
}

void remote_bitmap_fill_rectangle(
    RemoteBitmap* bitmap,
    int x,
    int y,
    int width,
    int height,
    RemoteBitmapColour colour) {
    if(width <= 0 || height <= 0) {
        return;
    }
    /* Clip in integer arithmetic before the loop so a far away rectangle
     * costs nothing and a huge one cannot overflow the coordinates. */
    int left = x < 0 ? 0 : x;
    int top = y < 0 ? 0 : y;
    int right = (x > REMOTE_BITMAP_WIDTH - width) ? REMOTE_BITMAP_WIDTH : x + width;
    int bottom = (y > REMOTE_BITMAP_HEIGHT - height) ? REMOTE_BITMAP_HEIGHT : y + height;
    for(int row = top; row < bottom; row++) {
        for(int column = left; column < right; column++) {
            remote_bitmap_set_pixel(bitmap, column, row, colour);
        }
    }
}
