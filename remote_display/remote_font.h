/*
 * A fixed cell bitmap font for status text.
 *
 * Five by seven glyphs on a six pixel advance, drawn at an integer scale, so
 * the layout arithmetic in KE2 is exact and a stranger can read a status
 * code at arm's length once the scale is chosen on the panel. One case only:
 * lower case is folded to upper case, which keeps the table small and the
 * text legible at a distance. A character with no glyph (anything outside
 * printable ASCII, and the few punctuation marks the table omits) occupies
 * its cell and draws nothing, so a stray byte can never index outside the
 * table or draw garbage.
 *
 * The glyphs are this project's own; there is nothing to verify them
 * against but the eye, and a test states the shape of one of them so the
 * cell geometry is proven rather than assumed.
 */
#pragma once

#include "remote_bitmap.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REMOTE_FONT_GLYPH_WIDTH 5
#define REMOTE_FONT_GLYPH_HEIGHT 7
#define REMOTE_FONT_ADVANCE (REMOTE_FONT_GLYPH_WIDTH + 1)

/* Draws text with its top left cell at (x, y); each glyph pixel becomes a
 * scale by scale square. Clipped to the panel. A scale under one draws
 * nothing. */
void remote_font_draw_text(
    RemoteBitmap* bitmap,
    int x,
    int y,
    const char* text,
    int scale,
    RemoteBitmapColour colour);

/* The width the text occupies at that scale, including the trailing gap of
 * the last cell, so cells tile. */
int remote_font_text_width(const char* text, int scale);

#ifdef __cplusplus
}
#endif
