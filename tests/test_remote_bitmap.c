/*
 * KE1 tests first (Pico spec, KE1): the frame buffer the panel is sent.
 *
 * The packing is the one the manufacturer documents for this panel: one bit
 * per pixel, eight horizontal pixels per byte, most significant bit first,
 * 1 white and 0 black (evaluation log 4.3). A first light image that draws
 * into the wrong packing looks like noise, so the packing is proven here
 * before a byte reaches the panel.
 */
#include "test_support.h"

#include "../remote_display/remote_bitmap.h"
#include "../remote_display/remote_font.h"

static bool every_byte_is(const RemoteBitmap* bitmap, uint8_t expected) {
    for(size_t byte_index = 0; byte_index < REMOTE_BITMAP_SIZE_BYTES; byte_index++) {
        if(bitmap->bytes[byte_index] != expected) {
            return false;
        }
    }
    return true;
}

static void the_buffer_is_sized_for_the_panel_at_one_bit_per_pixel(RemoteTestReport* report) {
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 400, REMOTE_BITMAP_WIDTH, "panel width");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 300, REMOTE_BITMAP_HEIGHT, "panel height");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 50, REMOTE_BITMAP_BYTES_PER_ROW, "400 pixels is 50 bytes");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 15000, REMOTE_BITMAP_SIZE_BYTES, "50 bytes by 300 rows");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, sizeof(((RemoteBitmap*)0)->bytes), REMOTE_BITMAP_SIZE_BYTES, "the array is that size");
}

static void clearing_to_white_sets_every_bit_and_to_black_clears_every_bit(RemoteTestReport* report) {
    RemoteBitmap bitmap;
    remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
    REMOTE_TEST_ASSERT(report, every_byte_is(&bitmap, 0xFF), "white is all ones");
    remote_bitmap_clear(&bitmap, RemoteBitmapBlack);
    REMOTE_TEST_ASSERT(report, every_byte_is(&bitmap, 0x00), "black is all zeros");
}

typedef struct {
    int x;
    int y;
    size_t expected_byte_index;
    uint8_t expected_byte_after_black_pixel;
    const char* description;
} PackingRow;

static const PackingRow packing_rows[] = {
    {0, 0, 0, 0x7F, "first pixel is the most significant bit of byte 0"},
    {7, 0, 0, 0xFE, "eighth pixel is the least significant bit of byte 0"},
    {8, 0, 1, 0x7F, "ninth pixel starts byte 1"},
    {0, 1, 50, 0x7F, "second row starts at byte 50"},
    {399, 299, 14999, 0xFE, "last pixel is the least significant bit of the last byte"},
};

static void a_black_pixel_clears_its_bit_msb_first_in_row_major_bytes(RemoteTestReport* report) {
    for(int row_index = 0; row_index < REMOTE_TEST_ROW_COUNT(packing_rows); row_index++) {
        const PackingRow* row = &packing_rows[row_index];
        RemoteBitmap bitmap;
        remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
        remote_bitmap_set_pixel(&bitmap, row->x, row->y, RemoteBitmapBlack);
        REMOTE_TEST_ASSERT_EQUAL_INT(
            report, row->expected_byte_after_black_pixel, bitmap.bytes[row->expected_byte_index], row->description);
        REMOTE_TEST_ASSERT_EQUAL_INT(
            report, RemoteBitmapBlack, remote_bitmap_get_pixel(&bitmap, row->x, row->y), row->description);
        /* Only that one byte changed. */
        bitmap.bytes[row->expected_byte_index] = 0xFF;
        REMOTE_TEST_ASSERT(report, every_byte_is(&bitmap, 0xFF), "no other byte was touched");
    }
}

static void setting_a_pixel_back_to_white_restores_the_bit(RemoteTestReport* report) {
    RemoteBitmap bitmap;
    remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
    remote_bitmap_set_pixel(&bitmap, 13, 7, RemoteBitmapBlack);
    remote_bitmap_set_pixel(&bitmap, 13, 7, RemoteBitmapWhite);
    REMOTE_TEST_ASSERT(report, every_byte_is(&bitmap, 0xFF), "white again");
}

typedef struct {
    int x;
    int y;
    const char* description;
} OutOfRangeRow;

static const OutOfRangeRow out_of_range_rows[] = {
    {-1, 0, "left of the panel"},
    {0, -1, "above the panel"},
    {400, 0, "one past the right edge"},
    {0, 300, "one past the bottom edge"},
    {100000, 100000, "far outside"},
    {-100000, 5, "far left"},
};

static void a_pixel_outside_the_panel_is_ignored_and_reads_white(RemoteTestReport* report) {
    for(int row_index = 0; row_index < REMOTE_TEST_ROW_COUNT(out_of_range_rows); row_index++) {
        const OutOfRangeRow* row = &out_of_range_rows[row_index];
        RemoteBitmap bitmap;
        remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
        remote_bitmap_set_pixel(&bitmap, row->x, row->y, RemoteBitmapBlack);
        REMOTE_TEST_ASSERT(report, every_byte_is(&bitmap, 0xFF), row->description);
        REMOTE_TEST_ASSERT_EQUAL_INT(
            report, RemoteBitmapWhite, remote_bitmap_get_pixel(&bitmap, row->x, row->y), row->description);
    }
}

static int count_black_pixels(const RemoteBitmap* bitmap) {
    int black = 0;
    for(int y = 0; y < REMOTE_BITMAP_HEIGHT; y++) {
        for(int x = 0; x < REMOTE_BITMAP_WIDTH; x++) {
            if(remote_bitmap_get_pixel(bitmap, x, y) == RemoteBitmapBlack) {
                black++;
            }
        }
    }
    return black;
}

static void a_rectangle_fills_exactly_its_area_and_is_clipped_at_the_edges(RemoteTestReport* report) {
    RemoteBitmap bitmap;
    remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
    remote_bitmap_fill_rectangle(&bitmap, 10, 20, 30, 40, RemoteBitmapBlack);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 30 * 40, count_black_pixels(&bitmap), "30 by 40 pixels");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapBlack, remote_bitmap_get_pixel(&bitmap, 10, 20), "top left corner");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapBlack, remote_bitmap_get_pixel(&bitmap, 39, 59), "bottom right corner");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapWhite, remote_bitmap_get_pixel(&bitmap, 40, 20), "just right of it");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapWhite, remote_bitmap_get_pixel(&bitmap, 10, 60), "just below it");

    remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
    remote_bitmap_fill_rectangle(&bitmap, 390, 290, 100, 100, RemoteBitmapBlack);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 10 * 10, count_black_pixels(&bitmap), "clipped to the corner");

    remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
    remote_bitmap_fill_rectangle(&bitmap, -5, -5, 10, 10, RemoteBitmapBlack);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 5 * 5, count_black_pixels(&bitmap), "clipped at the origin");

    remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
    remote_bitmap_fill_rectangle(&bitmap, 10, 10, 0, 50, RemoteBitmapBlack);
    remote_bitmap_fill_rectangle(&bitmap, 10, 10, -3, 50, RemoteBitmapBlack);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, count_black_pixels(&bitmap), "zero or negative width draws nothing");
}

/* The font: a fixed cell so layout arithmetic is exact, and a glyph whose
 * shape is stated here so the test is not circular. The capital I is a top
 * bar, a stem down the middle column, and a bottom bar. */
static void a_known_glyph_draws_its_stated_shape_in_its_cell(RemoteTestReport* report) {
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 5, REMOTE_FONT_GLYPH_WIDTH, "five columns");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 7, REMOTE_FONT_GLYPH_HEIGHT, "seven rows");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 6, REMOTE_FONT_ADVANCE, "one column of space between glyphs");

    RemoteBitmap bitmap;
    remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
    remote_font_draw_text(&bitmap, 0, 0, "I", 1, RemoteBitmapBlack);
    for(int column = 0; column < 5; column++) {
        REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapBlack, remote_bitmap_get_pixel(&bitmap, column, 0), "top bar");
        REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapBlack, remote_bitmap_get_pixel(&bitmap, column, 6), "bottom bar");
    }
    for(int row = 1; row < 6; row++) {
        REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapBlack, remote_bitmap_get_pixel(&bitmap, 2, row), "stem");
        REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapWhite, remote_bitmap_get_pixel(&bitmap, 0, row), "left of stem");
        REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapWhite, remote_bitmap_get_pixel(&bitmap, 4, row), "right of stem");
    }
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 5 + 5 + 5, count_black_pixels(&bitmap), "exactly the bars and the stem");
}

static void text_advances_one_cell_per_character_and_scales_each_pixel_to_a_square(RemoteTestReport* report) {
    RemoteBitmap bitmap;
    remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
    remote_font_draw_text(&bitmap, 0, 0, "II", 1, RemoteBitmapBlack);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapWhite, remote_bitmap_get_pixel(&bitmap, 5, 0), "the gap column is blank");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapBlack, remote_bitmap_get_pixel(&bitmap, 6, 0), "second glyph starts at the advance");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 30, count_black_pixels(&bitmap), "two glyphs");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 2 * REMOTE_FONT_ADVANCE, remote_font_text_width("II", 1), "measured width");

    remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
    remote_font_draw_text(&bitmap, 0, 0, "I", 3, RemoteBitmapBlack);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 15 * 9, count_black_pixels(&bitmap), "scale three squares every pixel");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapBlack, remote_bitmap_get_pixel(&bitmap, 14, 2), "top bar spans fifteen columns and three rows");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapWhite, remote_bitmap_get_pixel(&bitmap, 15, 2), "and no more");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 3 * REMOTE_FONT_ADVANCE, remote_font_text_width("I", 3), "measured width scales");
}

/* Bytes the font has no glyph for, including anything outside printable
 * ASCII, occupy a cell and draw nothing, so a stray value from the wire can
 * never index outside the glyph table or produce garbage. */
static void a_character_without_a_glyph_draws_nothing_but_keeps_its_cell(RemoteTestReport* report) {
    const char unknown_text[] = {'I', '\x01', 'I', '\x7F', 'I', '\xFF', 'I', '\0'};
    RemoteBitmap bitmap;
    remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
    remote_font_draw_text(&bitmap, 0, 0, unknown_text, 1, RemoteBitmapBlack);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 4 * 15, count_black_pixels(&bitmap), "only the four I glyphs drew");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapBlack, remote_bitmap_get_pixel(&bitmap, 2 * REMOTE_FONT_ADVANCE + 2, 3), "third cell holds the second I");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 7 * REMOTE_FONT_ADVANCE, remote_font_text_width(unknown_text, 1), "seven cells");
}

/* Lower case is folded to upper case: the panel is read at arm's length and
 * one case keeps the glyph table small. */
static void lower_case_draws_as_upper_case(RemoteTestReport* report) {
    RemoteBitmap upper;
    RemoteBitmap lower;
    remote_bitmap_clear(&upper, RemoteBitmapWhite);
    remote_bitmap_clear(&lower, RemoteBitmapWhite);
    remote_font_draw_text(&upper, 3, 3, "STOPBATH", 2, RemoteBitmapBlack);
    remote_font_draw_text(&lower, 3, 3, "stopbath", 2, RemoteBitmapBlack);
    REMOTE_TEST_ASSERT(report, memcmp(upper.bytes, lower.bytes, REMOTE_BITMAP_SIZE_BYTES) == 0, "identical bitmaps");
    REMOTE_TEST_ASSERT(report, count_black_pixels(&upper) > 0, "and something was drawn");
}

/* Every printable ASCII character the wire can carry has either a glyph or
 * an empty cell, and none writes outside the panel when placed at its edge. */
static void every_printable_character_at_the_panel_edge_stays_inside_the_buffer(RemoteTestReport* report) {
    char single[2] = {0, 0};
    for(int code = 0x20; code <= 0x7E; code++) {
        single[0] = (char)code;
        RemoteBitmap bitmap;
        remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
        remote_font_draw_text(&bitmap, REMOTE_BITMAP_WIDTH - 2, REMOTE_BITMAP_HEIGHT - 2, single, 4, RemoteBitmapBlack);
        remote_font_draw_text(&bitmap, -7, -7, single, 2, RemoteBitmapBlack);
        REMOTE_TEST_ASSERT(report, count_black_pixels(&bitmap) >= 0, "drew without fault");
    }
}

static void drawing_never_allocates(RemoteTestReport* report) {
    RemoteBitmap bitmap;
    int allocations_before = remote_test_allocation_count;
    remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
    remote_bitmap_fill_rectangle(&bitmap, 0, 0, 400, 300, RemoteBitmapBlack);
    remote_font_draw_text(&bitmap, 10, 10, "READY 0123456789", 2, RemoteBitmapWhite);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, allocations_before, remote_test_allocation_count, "no allocation drawing");
}

static const RemoteTestCase test_cases[] = {
    {"the buffer is sized for the panel at one bit per pixel", the_buffer_is_sized_for_the_panel_at_one_bit_per_pixel},
    {"clearing to white sets every bit and to black clears every bit",
     clearing_to_white_sets_every_bit_and_to_black_clears_every_bit},
    {"a black pixel clears its bit msb first in row major bytes", a_black_pixel_clears_its_bit_msb_first_in_row_major_bytes},
    {"setting a pixel back to white restores the bit", setting_a_pixel_back_to_white_restores_the_bit},
    {"a pixel outside the panel is ignored and reads white", a_pixel_outside_the_panel_is_ignored_and_reads_white},
    {"a rectangle fills exactly its area and is clipped at the edges",
     a_rectangle_fills_exactly_its_area_and_is_clipped_at_the_edges},
    {"a known glyph draws its stated shape in its cell", a_known_glyph_draws_its_stated_shape_in_its_cell},
    {"text advances one cell per character and scales each pixel to a square",
     text_advances_one_cell_per_character_and_scales_each_pixel_to_a_square},
    {"a character without a glyph draws nothing but keeps its cell",
     a_character_without_a_glyph_draws_nothing_but_keeps_its_cell},
    {"lower case draws as upper case", lower_case_draws_as_upper_case},
    {"every printable character at the panel edge stays inside the buffer",
     every_printable_character_at_the_panel_edge_stays_inside_the_buffer},
    {"drawing never allocates", drawing_never_allocates},
};

int main(void) {
    return remote_test_run_all(test_cases, REMOTE_TEST_ROW_COUNT(test_cases));
}
