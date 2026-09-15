/*
 * KE5 tests first (Pico spec, KE5): the code on this panel. Version
 * selection for the two real payload shapes, module sizing that fills the
 * code square while keeping whole pixels per module and a full quiet zone,
 * refusal of what cannot be shown, and the layout drawing a real code in
 * place of the KE2 frame. The matrices themselves are proven against an
 * independent encoder in test_remote_qr_vectors.c.
 */
#include "test_support.h"

#include "../remote_display/remote_display_fixtures.h"
#include "../remote_display/remote_display_layout.h"
#include "../remote_display/remote_qr.h"

static int count_black_pixels_in(const RemoteBitmap* bitmap, RemoteLayoutRectangle area) {
    int black = 0;
    for(int y = area.y; y < area.y + area.height; y++) {
        for(int x = area.x; x < area.x + area.width; x++) {
            if(remote_bitmap_get_pixel(bitmap, x, y) == RemoteBitmapBlack) {
                black++;
            }
        }
    }
    return black;
}

static void the_real_wifi_payload_is_version_three_and_the_gallery_address_version_one(RemoteTestReport* report) {
    RemoteQrMatrix matrix;
    REMOTE_TEST_ASSERT(report, remote_qr_encode(REMOTE_DISPLAY_FIXTURE_WIFI_PAYLOAD, &matrix), "53 byte wifi payload encodes");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 29, matrix.side, "version three");
    REMOTE_TEST_ASSERT(report, remote_qr_encode(REMOTE_DISPLAY_FIXTURE_GUEST_PAYLOAD, &matrix), "gallery address encodes");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 21, matrix.side, "version one");
    /* The finder pattern: the top left module is dark, the one inside the
     * ring is light. A transposed or shifted symbol fails this. */
    REMOTE_TEST_ASSERT(report, remote_qr_module_is_dark(&matrix, 0, 0), "finder corner dark");
    REMOTE_TEST_ASSERT(report, !remote_qr_module_is_dark(&matrix, 1, 1), "finder ring light");
    REMOTE_TEST_ASSERT(report, !remote_qr_module_is_dark(&matrix, -1, 0) && !remote_qr_module_is_dark(&matrix, 21, 0), "outside the symbol is light");
}

typedef struct {
    int side;
    int expected_pixels_per_module;
    const char* description;
} SizingRow;

/* The code square is 259 pixels (REMOTE_LAYOUT_CODE_SIDE) and the quiet zone
 * four modules a side, so the module is the largest whole number of pixels
 * with (side + 8) modules fitting. */
static const SizingRow sizing_rows[] = {
    {21, 8, "version one: 29 modules across, 8 pixels each is 232"},
    {25, 7, "version two: 33 across, 7 pixels each is 231"},
    {29, 7, "version three: 37 across, 7 pixels each is exactly 259"},
    {33, 6, "version four: 41 across, 6 pixels each is 246"},
    {53, 4, "version nine: 61 across, 4 pixels each is 244"},
};

static void the_module_is_the_largest_whole_pixel_size_that_fits_with_the_quiet_zone(RemoteTestReport* report) {
    for(int row_index = 0; row_index < REMOTE_TEST_ROW_COUNT(sizing_rows); row_index++) {
        const SizingRow* row = &sizing_rows[row_index];
        int pixels = remote_qr_pixels_per_module(row->side, REMOTE_LAYOUT_CODE_SIDE);
        REMOTE_TEST_ASSERT_EQUAL_INT(report, row->expected_pixels_per_module, pixels, row->description);
        REMOTE_TEST_ASSERT(report, (row->side + 2 * REMOTE_QR_QUIET_ZONE_MODULES) * pixels <= REMOTE_LAYOUT_CODE_SIDE, "fits");
        REMOTE_TEST_ASSERT(report, pixels >= REMOTE_QR_MINIMUM_PIXELS_PER_MODULE, "never below the minimum");
    }
    /* The ceiling version is the largest whose modules stay at the minimum
     * size in this square; one more would fall below it. */
    int ceiling_side = REMOTE_QR_MAX_VERSION * 4 + 17;
    REMOTE_TEST_ASSERT(report, remote_qr_pixels_per_module(ceiling_side, REMOTE_LAYOUT_CODE_SIDE) >= REMOTE_QR_MINIMUM_PIXELS_PER_MODULE, "ceiling holds the minimum");
    REMOTE_TEST_ASSERT(report, remote_qr_pixels_per_module(ceiling_side + 4, REMOTE_LAYOUT_CODE_SIDE) < REMOTE_QR_MINIMUM_PIXELS_PER_MODULE, "the next version would not");
}

static void a_drawn_code_is_centred_with_a_light_quiet_zone_and_nothing_outside_the_area(RemoteTestReport* report) {
    RemoteQrMatrix matrix;
    remote_qr_encode(REMOTE_DISPLAY_FIXTURE_WIFI_PAYLOAD, &matrix);
    RemoteBitmap bitmap;
    remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
    RemoteLayoutRectangle area = {40, 30, REMOTE_LAYOUT_CODE_SIDE, REMOTE_LAYOUT_CODE_SIDE};
    remote_qr_draw(&matrix, &bitmap, area);

    int pixels = remote_qr_pixels_per_module(matrix.side, area.width);
    int symbol_pixels = matrix.side * pixels;
    int origin_x = area.x + (area.width - symbol_pixels) / 2;
    int origin_y = area.y + (area.height - symbol_pixels) / 2;
    REMOTE_TEST_ASSERT(report, origin_x - area.x >= REMOTE_QR_QUIET_ZONE_MODULES * pixels, "quiet zone on the left");
    REMOTE_TEST_ASSERT(report, area.x + area.width - (origin_x + symbol_pixels) >= REMOTE_QR_QUIET_ZONE_MODULES * pixels, "quiet zone on the right");

    /* The top left finder module fills exactly pixels by pixels at the origin. */
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapBlack, remote_bitmap_get_pixel(&bitmap, origin_x, origin_y), "finder corner");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapBlack, remote_bitmap_get_pixel(&bitmap, origin_x + pixels - 1, origin_y + pixels - 1), "whole module");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapWhite, remote_bitmap_get_pixel(&bitmap, origin_x + pixels, origin_y + pixels), "the ring module is light");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteBitmapWhite, remote_bitmap_get_pixel(&bitmap, origin_x - 1, origin_y), "quiet zone is light");

    /* Nothing outside the area, and the quiet zone band is entirely light. */
    RemoteLayoutRectangle whole = {0, 0, REMOTE_BITMAP_WIDTH, REMOTE_BITMAP_HEIGHT};
    RemoteLayoutRectangle symbol = {origin_x, origin_y, symbol_pixels, symbol_pixels};
    REMOTE_TEST_ASSERT_EQUAL_INT(report, count_black_pixels_in(&bitmap, symbol), count_black_pixels_in(&bitmap, whole), "every dark pixel is inside the symbol");
    REMOTE_TEST_ASSERT(report, count_black_pixels_in(&bitmap, symbol) > 0, "and there are some");
}

static void an_empty_or_oversized_payload_is_refused_not_truncated(RemoteTestReport* report) {
    RemoteQrMatrix matrix;
    REMOTE_TEST_ASSERT(report, !remote_qr_encode("", &matrix), "empty refused");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, matrix.side, "and the matrix is empty");
    REMOTE_TEST_ASSERT(report, !remote_qr_can_encode(""), "can_encode agrees");

    /* Beyond the ceiling version's byte capacity: refused whole, never a
     * code of the first part of it. */
    char oversized[400];
    memset(oversized, 'x', sizeof(oversized) - 1);
    oversized[sizeof(oversized) - 1] = '\0';
    REMOTE_TEST_ASSERT(report, !remote_qr_encode(oversized, &matrix), "oversized refused");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, matrix.side, "matrix empty");
    RemoteBitmap bitmap;
    remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
    RemoteLayoutRectangle area = {0, 0, REMOTE_LAYOUT_CODE_SIDE, REMOTE_LAYOUT_CODE_SIDE};
    remote_qr_draw(&matrix, &bitmap, area);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, count_black_pixels_in(&bitmap, area), "an empty matrix draws nothing");

    /* The ceiling's byte capacity, probed against the encoder: 230 bytes at
     * the lowest error correction, one more refused. Far above any Wi-Fi
     * payload the standard grammar can produce, and stated here so a change
     * to the ceiling shows up as a number. */
    char at_capacity[232];
    memset(at_capacity, 'x', 230);
    at_capacity[230] = '\0';
    REMOTE_TEST_ASSERT(report, remote_qr_can_encode(at_capacity), "230 bytes encodes at the ceiling");
    memset(at_capacity, 'x', 231);
    at_capacity[231] = '\0';
    REMOTE_TEST_ASSERT(report, !remote_qr_can_encode(at_capacity), "231 bytes is refused");
}

static void the_layout_draws_a_real_code_for_a_page_and_a_distinct_error_when_it_cannot(RemoteTestReport* report) {
    RemoteDisplayState wifi;
    remote_display_state_initialise(&wifi);
    wifi.link_connected = true;
    wifi.status = RemoteDisplayStatusPresenting;
    wifi.page = RemoteDisplayPageWifi;
    strncpy(wifi.payload, REMOTE_DISPLAY_FIXTURE_WIFI_PAYLOAD, REMOTE_DISPLAY_PAYLOAD_CAPACITY - 1);
    RemoteBitmap bitmap;
    remote_display_layout_render(&wifi, &bitmap);

    /* The same code, drawn by the wrapper into the same square, pixel for
     * pixel: the layout adds nothing to and takes nothing from the code. */
    RemoteQrMatrix matrix;
    remote_qr_encode(wifi.payload, &matrix);
    RemoteLayoutRectangle code_region = remote_display_layout_region(RemoteLayoutRegionCode);
    int inset = (code_region.width - REMOTE_LAYOUT_CODE_SIDE) / 2;
    RemoteLayoutRectangle square = {code_region.x + inset, code_region.y + inset, REMOTE_LAYOUT_CODE_SIDE, REMOTE_LAYOUT_CODE_SIDE};
    RemoteBitmap expected;
    remote_bitmap_clear(&expected, RemoteBitmapWhite);
    remote_qr_draw(&matrix, &expected, square);
    bool identical = true;
    for(int y = code_region.y; y < code_region.y + code_region.height; y++) {
        for(int x = code_region.x; x < code_region.x + code_region.width; x++) {
            if(remote_bitmap_get_pixel(&bitmap, x, y) != remote_bitmap_get_pixel(&expected, x, y)) {
                identical = false;
            }
        }
    }
    REMOTE_TEST_ASSERT(report, identical, "the code region is exactly the drawn code");

    /* A page whose payload cannot be encoded shows a distinct error in the
     * square. A 256 byte payload fits (above), so the refusal is exercised
     * with the one payload the wrapper always refuses: an empty one. */
    RemoteDisplayState empty_payload = wifi;
    empty_payload.payload[0] = '\0';
    RemoteBitmap refused;
    remote_display_layout_render(&empty_payload, &refused);
    RemoteBitmap plain;
    RemoteDisplayState no_page = wifi;
    no_page.page = RemoteDisplayPageNone;
    remote_display_layout_render(&no_page, &plain);
    REMOTE_TEST_ASSERT(report, count_black_pixels_in(&refused, code_region) > 0, "something is drawn in place of the code");
    REMOTE_TEST_ASSERT(report, count_black_pixels_in(&refused, code_region) < count_black_pixels_in(&bitmap, code_region) / 4, "and it is not a code");
    REMOTE_TEST_ASSERT(report, memcmp(refused.bytes, bitmap.bytes, REMOTE_BITMAP_SIZE_BYTES) != 0, "distinct from the code");
    REMOTE_TEST_ASSERT(report, memcmp(refused.bytes, plain.bytes, REMOTE_BITMAP_SIZE_BYTES) != 0, "distinct from no page");
}

static void encoding_and_drawing_never_allocate(RemoteTestReport* report) {
    int allocations_before = remote_test_allocation_count;
    RemoteQrMatrix matrix;
    remote_qr_encode(REMOTE_DISPLAY_FIXTURE_WIFI_PAYLOAD, &matrix);
    RemoteBitmap bitmap;
    remote_bitmap_clear(&bitmap, RemoteBitmapWhite);
    RemoteLayoutRectangle area = {0, 0, REMOTE_LAYOUT_CODE_SIDE, REMOTE_LAYOUT_CODE_SIDE};
    remote_qr_draw(&matrix, &bitmap, area);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, allocations_before, remote_test_allocation_count, "no allocation");
}

static const RemoteTestCase test_cases[] = {
    {"the real wifi payload is version three and the gallery address version one",
     the_real_wifi_payload_is_version_three_and_the_gallery_address_version_one},
    {"the module is the largest whole pixel size that fits with the quiet zone",
     the_module_is_the_largest_whole_pixel_size_that_fits_with_the_quiet_zone},
    {"a drawn code is centred with a light quiet zone and nothing outside the area",
     a_drawn_code_is_centred_with_a_light_quiet_zone_and_nothing_outside_the_area},
    {"an empty or oversized payload is refused not truncated", an_empty_or_oversized_payload_is_refused_not_truncated},
    {"the layout draws a real code for a page and a distinct error when it cannot",
     the_layout_draws_a_real_code_for_a_page_and_a_distinct_error_when_it_cannot},
    {"encoding and drawing never allocate", encoding_and_drawing_never_allocate},
};

int main(void) {
    return remote_test_run_all(test_cases, REMOTE_TEST_ROW_COUNT(test_cases));
}
