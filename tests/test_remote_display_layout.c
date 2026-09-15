/*
 * KE2 tests first (Pico spec, KE2): state to layout for every fixture, the
 * safe fallback for an unknown value, truncation, and the region rules that
 * keep a guest's code still while other fields change.
 */
#include "test_support.h"

#include "../remote_display/remote_bitmap.h"
#include "../remote_display/remote_display_fixtures.h"
#include "../remote_display/remote_display_layout.h"

static RemoteDisplayState connected_state(int status, int page) {
    RemoteDisplayState display_state;
    remote_display_state_initialise(&display_state);
    display_state.link_connected = true;
    display_state.status = status;
    display_state.page = page;
    return display_state;
}

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

static RemoteLayoutRectangle whole_panel(void) {
    RemoteLayoutRectangle area = {0, 0, REMOTE_BITMAP_WIDTH, REMOTE_BITMAP_HEIGHT};
    return area;
}

static bool point_in(RemoteLayoutRectangle area, int x, int y) {
    return x >= area.x && x < area.x + area.width && y >= area.y && y < area.y + area.height;
}

/* Every pixel that differs between two renders lies inside one of the
 * regions in the mask. This is what makes a claimed region set trustworthy:
 * the layout may only touch what it says it touches. */
static bool differences_confined_to(const RemoteBitmap* before, const RemoteBitmap* after, unsigned region_mask) {
    for(int y = 0; y < REMOTE_BITMAP_HEIGHT; y++) {
        for(int x = 0; x < REMOTE_BITMAP_WIDTH; x++) {
            if(remote_bitmap_get_pixel(before, x, y) == remote_bitmap_get_pixel(after, x, y)) {
                continue;
            }
            bool covered = false;
            for(int region = 0; region < RemoteLayoutRegionCount; region++) {
                if((region_mask & (1u << region)) && point_in(remote_display_layout_region((RemoteLayoutRegion)region), x, y)) {
                    covered = true;
                    break;
                }
            }
            if(!covered) {
                return false;
            }
        }
    }
    return true;
}

static void every_fixture_renders_something(RemoteTestReport* report) {
    for(int fixture_index = 0; fixture_index < remote_display_fixture_count(); fixture_index++) {
        const RemoteDisplayFixture* fixture = &remote_display_fixtures()[fixture_index];
        RemoteBitmap bitmap;
        remote_display_layout_render(&fixture->display_state, &bitmap);
        REMOTE_TEST_ASSERT(report, count_black_pixels_in(&bitmap, whole_panel()) > 0, fixture->fixture_name);
    }
}

/* The regions tile the column and the code area without overlapping, start
 * on a byte boundary and span whole bytes, because the panel's partial
 * refresh addresses RAM in bytes of eight pixels and a region that is not
 * byte aligned would drag its neighbour's pixels into the refresh. */
static void regions_are_byte_aligned_inside_the_panel_and_disjoint(RemoteTestReport* report) {
    for(int region = 0; region < RemoteLayoutRegionCount; region++) {
        RemoteLayoutRectangle area = remote_display_layout_region((RemoteLayoutRegion)region);
        REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, area.x % 8, "x on a byte boundary");
        REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, area.width % 8, "width in whole bytes");
        REMOTE_TEST_ASSERT(report, area.x >= 0 && area.y >= 0, "inside the panel");
        REMOTE_TEST_ASSERT(report, area.x + area.width <= REMOTE_BITMAP_WIDTH, "inside the panel width");
        REMOTE_TEST_ASSERT(report, area.y + area.height <= REMOTE_BITMAP_HEIGHT, "inside the panel height");
        REMOTE_TEST_ASSERT(report, area.width > 0 && area.height > 0, "not empty");
        for(int other = region + 1; other < RemoteLayoutRegionCount; other++) {
            RemoteLayoutRectangle other_area = remote_display_layout_region((RemoteLayoutRegion)other);
            bool overlap = area.x < other_area.x + other_area.width && other_area.x < area.x + area.width &&
                           area.y < other_area.y + other_area.height && other_area.y < area.y + area.height;
            REMOTE_TEST_ASSERT(report, !overlap, "regions do not overlap");
        }
    }
}

static void an_unknown_status_renders_the_fallback_not_blank_or_another_status(RemoteTestReport* report) {
    RemoteDisplayState unknown = connected_state(REMOTE_DISPLAY_FIXTURE_UNKNOWN_STATUS, RemoteDisplayPageNone);
    RemoteDisplayState negative = connected_state(-5, RemoteDisplayPageNone);
    RemoteDisplayState ready = connected_state(RemoteDisplayStatusReady, RemoteDisplayPageNone);
    RemoteBitmap unknown_bitmap;
    RemoteBitmap negative_bitmap;
    RemoteBitmap ready_bitmap;
    remote_display_layout_render(&unknown, &unknown_bitmap);
    remote_display_layout_render(&negative, &negative_bitmap);
    remote_display_layout_render(&ready, &ready_bitmap);
    RemoteLayoutRectangle header = remote_display_layout_region(RemoteLayoutRegionHeader);
    REMOTE_TEST_ASSERT(report, count_black_pixels_in(&unknown_bitmap, header) > 0, "the header says something");
    REMOTE_TEST_ASSERT(
        report, memcmp(unknown_bitmap.bytes, negative_bitmap.bytes, REMOTE_BITMAP_SIZE_BYTES) == 0,
        "every out of range status renders the same fallback");
    REMOTE_TEST_ASSERT(
        report, memcmp(unknown_bitmap.bytes, ready_bitmap.bytes, REMOTE_BITMAP_SIZE_BYTES) != 0,
        "and it is not mistaken for ready");

    /* An unknown page draws no code, like NONE, rather than guessing one. */
    RemoteDisplayState unknown_page = connected_state(RemoteDisplayStatusPresenting, RemoteDisplayPageCount + 2);
    RemoteDisplayState no_page = connected_state(RemoteDisplayStatusPresenting, RemoteDisplayPageNone);
    RemoteBitmap unknown_page_bitmap;
    RemoteBitmap no_page_bitmap;
    remote_display_layout_render(&unknown_page, &unknown_page_bitmap);
    remote_display_layout_render(&no_page, &no_page_bitmap);
    REMOTE_TEST_ASSERT(
        report, memcmp(unknown_page_bitmap.bytes, no_page_bitmap.bytes, REMOTE_BITMAP_SIZE_BYTES) == 0,
        "unknown page renders as no page");
}

/* A value longer than its region is cut, never allowed to spill into a
 * neighbouring region, because the neighbour will not be refreshed when
 * this one changes. */
static void long_values_are_truncated_inside_their_region(RemoteTestReport* report) {
    RemoteDisplayState without_error = connected_state(RemoteDisplayStatusPresenting, RemoteDisplayPageWifi);
    RemoteDisplayState with_long_error = without_error;
    memset(with_long_error.error_code, 'W', REMOTE_DISPLAY_CODE_CAPACITY - 1);
    with_long_error.error_code[REMOTE_DISPLAY_CODE_CAPACITY - 1] = '\0';
    RemoteBitmap plain;
    RemoteBitmap long_error;
    remote_display_layout_render(&without_error, &plain);
    remote_display_layout_render(&with_long_error, &long_error);
    REMOTE_TEST_ASSERT(
        report, differences_confined_to(&plain, &long_error, 1u << RemoteLayoutRegionError),
        "a 32 character error code stays inside the error band");
    REMOTE_TEST_ASSERT(
        report, count_black_pixels_in(&long_error, remote_display_layout_region(RemoteLayoutRegionError)) > 0,
        "and something of it is drawn");

    RemoteDisplayState huge_count = without_error;
    huge_count.delivered_count = 4000000000u;
    RemoteBitmap huge;
    remote_display_layout_render(&huge_count, &huge);
    REMOTE_TEST_ASSERT(
        report, differences_confined_to(&plain, &huge, 1u << RemoteLayoutRegionDelivered),
        "an absurd count stays inside the count region");

    RemoteDisplayState long_payload = without_error;
    memset(long_payload.payload, 'P', REMOTE_DISPLAY_PAYLOAD_CAPACITY - 1);
    long_payload.payload[REMOTE_DISPLAY_PAYLOAD_CAPACITY - 1] = '\0';
    RemoteBitmap payload_bitmap;
    remote_display_layout_render(&long_payload, &payload_bitmap);
    REMOTE_TEST_ASSERT(
        report, differences_confined_to(&plain, &payload_bitmap, 1u << RemoteLayoutRegionCode),
        "a 256 byte payload stays inside the code region");
}

typedef struct {
    const char* description;
    RemoteDisplayState before;
    RemoteDisplayState after;
    unsigned expected_regions;
} RegionRow;

static RegionRow region_row(const char* description, RemoteDisplayState before, RemoteDisplayState after, unsigned expected) {
    RegionRow row = {description, before, after, expected};
    return row;
}

static void field_changes_map_to_their_regions_and_the_render_agrees(RemoteTestReport* report) {
    RemoteDisplayState presenting_wifi = connected_state(RemoteDisplayStatusPresenting, RemoteDisplayPageWifi);
    strncpy(presenting_wifi.payload, "WIFI:T:WPA;S:fixture;P:fixturepass;;", REMOTE_DISPLAY_PAYLOAD_CAPACITY - 1);

    RemoteDisplayState one_delivered = presenting_wifi;
    one_delivered.delivered_count = 1;

    RemoteDisplayState guest_joined = presenting_wifi;
    guest_joined.status = RemoteDisplayStatusGuestConnected;

    RemoteDisplayState guest_page = presenting_wifi;
    guest_page.page = RemoteDisplayPageGuest;
    strncpy(guest_page.payload, "HTTP://192.168.72.1/", REMOTE_DISPLAY_PAYLOAD_CAPACITY - 1);

    RemoteDisplayState new_payload = presenting_wifi;
    strncpy(new_payload.payload, "WIFI:T:WPA;S:another;P:anotherpass;;", REMOTE_DISPLAY_PAYLOAD_CAPACITY - 1);

    RemoteDisplayState with_error = presenting_wifi;
    strncpy(with_error.error_code, "ACTIVE", REMOTE_DISPLAY_CODE_CAPACITY - 1);

    RemoteDisplayState ready = connected_state(RemoteDisplayStatusReady, RemoteDisplayPageNone);

    RegionRow rows[] = {
        region_row("a delivered count change touches only the count region", presenting_wifi, one_delivered, 1u << RemoteLayoutRegionDelivered),
        region_row("a status change touches the header and the hint, never the code", presenting_wifi, guest_joined, (1u << RemoteLayoutRegionHeader) | (1u << RemoteLayoutRegionHint)),
        region_row("a page change touches the code, the page label and the hint", presenting_wifi, guest_page, (1u << RemoteLayoutRegionCode) | (1u << RemoteLayoutRegionPage) | (1u << RemoteLayoutRegionHint)),
        region_row("a payload change touches only the code", presenting_wifi, new_payload, 1u << RemoteLayoutRegionCode),
        region_row("an error touches only the error band", presenting_wifi, with_error, 1u << RemoteLayoutRegionError),
        region_row("no change touches nothing", presenting_wifi, presenting_wifi, 0u),
        /* The count is hidden while ready and appears with the session, so it is
         * part of this change; the error band alone is untouched. */
        region_row("leaving idle for a session touches everything but the error band", ready, presenting_wifi, (1u << RemoteLayoutRegionHeader) | (1u << RemoteLayoutRegionCode) | (1u << RemoteLayoutRegionPage) | (1u << RemoteLayoutRegionHint) | (1u << RemoteLayoutRegionDelivered)),
    };
    for(int row_index = 0; row_index < REMOTE_TEST_ROW_COUNT(rows); row_index++) {
        const RegionRow* row = &rows[row_index];
        unsigned claimed = remote_display_layout_changed_regions(&row->before, &row->after);
        REMOTE_TEST_ASSERT_EQUAL_INT(report, row->expected_regions, claimed, row->description);
        RemoteBitmap before;
        RemoteBitmap after;
        remote_display_layout_render(&row->before, &before);
        remote_display_layout_render(&row->after, &after);
        REMOTE_TEST_ASSERT(report, differences_confined_to(&before, &after, claimed), row->description);
    }
}

/* The code region must never change for a change to any field other than
 * page or payload: a guest part way through scanning must not have the
 * code move (spec 2.6). Proven across every pair of fixtures that share a
 * page and payload. */
static void the_code_region_is_identical_across_states_with_the_same_page_and_payload(RemoteTestReport* report) {
    RemoteLayoutRectangle code = remote_display_layout_region(RemoteLayoutRegionCode);
    int pairs_checked = 0;
    for(int first = 0; first < remote_display_fixture_count(); first++) {
        for(int second = first + 1; second < remote_display_fixture_count(); second++) {
            const RemoteDisplayState* a = &remote_display_fixtures()[first].display_state;
            const RemoteDisplayState* b = &remote_display_fixtures()[second].display_state;
            if(!a->link_connected || !b->link_connected || a->show_diagnostics || b->show_diagnostics) {
                continue;
            }
            if(a->page != b->page || strcmp(a->payload, b->payload) != 0) {
                continue;
            }
            RemoteBitmap bitmap_a;
            RemoteBitmap bitmap_b;
            remote_display_layout_render(a, &bitmap_a);
            remote_display_layout_render(b, &bitmap_b);
            for(int y = code.y; y < code.y + code.height; y++) {
                for(int x = code.x; x < code.x + code.width; x++) {
                    if(remote_bitmap_get_pixel(&bitmap_a, x, y) != remote_bitmap_get_pixel(&bitmap_b, x, y)) {
                        REMOTE_TEST_ASSERT(report, false, remote_display_fixtures()[second].fixture_name);
                        y = code.y + code.height;
                        break;
                    }
                }
            }
            pairs_checked++;
        }
    }
    REMOTE_TEST_ASSERT(report, pairs_checked > 3, "the fixtures include such pairs");
}

/* A link screen replaces the whole panel; the region model does not apply
 * and every region is reported changed so the policy does a full refresh. */
static void a_link_state_change_reports_every_region(RemoteTestReport* report) {
    RemoteDisplayState connected = connected_state(RemoteDisplayStatusReady, RemoteDisplayPageNone);
    RemoteDisplayState disconnected = connected;
    disconnected.link_connected = false;
    unsigned all = (1u << RemoteLayoutRegionCount) - 1u;
    REMOTE_TEST_ASSERT_EQUAL_INT(report, all, remote_display_layout_changed_regions(&connected, &disconnected), "dropping the link");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, all, remote_display_layout_changed_regions(&disconnected, &connected), "regaining it");
    RemoteDisplayState diagnostics = connected;
    diagnostics.show_diagnostics = true;
    REMOTE_TEST_ASSERT_EQUAL_INT(report, all, remote_display_layout_changed_regions(&connected, &diagnostics), "showing diagnostics");
    RemoteDisplayState incompatible = disconnected;
    incompatible.link_incompatible = true;
    REMOTE_TEST_ASSERT_EQUAL_INT(report, all, remote_display_layout_changed_regions(&disconnected, &incompatible), "incompatible differs from disconnected");
}

static void rendering_never_allocates(RemoteTestReport* report) {
    RemoteBitmap bitmap;
    int allocations_before = remote_test_allocation_count;
    for(int fixture_index = 0; fixture_index < remote_display_fixture_count(); fixture_index++) {
        remote_display_layout_render(&remote_display_fixtures()[fixture_index].display_state, &bitmap);
    }
    REMOTE_TEST_ASSERT_EQUAL_INT(report, allocations_before, remote_test_allocation_count, "no allocation rendering");
}

static const RemoteTestCase test_cases[] = {
    {"every fixture renders something", every_fixture_renders_something},
    {"regions are byte aligned inside the panel and disjoint", regions_are_byte_aligned_inside_the_panel_and_disjoint},
    {"an unknown status renders the fallback not blank or another status",
     an_unknown_status_renders_the_fallback_not_blank_or_another_status},
    {"long values are truncated inside their region", long_values_are_truncated_inside_their_region},
    {"field changes map to their regions and the render agrees", field_changes_map_to_their_regions_and_the_render_agrees},
    {"the code region is identical across states with the same page and payload",
     the_code_region_is_identical_across_states_with_the_same_page_and_payload},
    {"a link state change reports every region", a_link_state_change_reports_every_region},
    {"rendering never allocates", rendering_never_allocates},
};

int main(void) {
    return remote_test_run_all(test_cases, REMOTE_TEST_ROW_COUNT(test_cases));
}
