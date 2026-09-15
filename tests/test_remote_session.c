/*
 * KE3 tests first (Pico spec, KE3): the client session. Handshake, wholesale
 * replacement, incompatible version, not connected on drop, no queueing
 * across a disconnection, the guard flags fixed true, HELLO's token and lock
 * value, and the Key1 mapping of spec 2.3.
 */
#include "test_support.h"

#include "../remote_display/remote_display_layout.h"
#include "../session/remote_session.h"

static const char* const READY_RECORD = "DISPLAY status=READY page=NONE payload= delivered=0 error=NONE\n";
static const char* const WIFI_RECORD =
    "DISPLAY status=PRESENTING page=WIFI payload=WIFI%3AT%3AWPA%3BS%3Afix%3BP%3Apass%3B%3B delivered=2 error=NONE\n";
static const char* const GUEST_RECORD =
    "DISPLAY status=GUEST_CONNECTED page=GUEST payload=HTTP%3A%2F%2F192.168.72.1%2F delivered=3 error=NONE\n";
static const char* const BAD_VERSION_RECORD = "DISPLAY status=READY page=NONE payload= delivered=0 error=BAD_VERSION\n";
static const char* const GUARD_ERROR_RECORD = "DISPLAY status=READY page=NONE payload= delivered=0 error=GUARD\n";

static void feed(RemoteSession* session, const char* text) {
    remote_session_receive(session, (const uint8_t*)text, strlen(text));
}

/* Drains the output into a terminated string the assertions can read. */
static size_t drain(RemoteSession* session, char* destination, size_t capacity) {
    size_t taken = remote_session_take_output(session, (uint8_t*)destination, capacity - 1);
    destination[taken] = '\0';
    return taken;
}

static RemoteSession connected_session(const char* record) {
    RemoteSession session;
    remote_session_initialise(&session);
    remote_session_port_opened(&session);
    char discard[512];
    drain(&session, discard, sizeof(discard));
    feed(&session, record);
    return session;
}

static void opening_the_port_sends_hello_with_the_fixed_token_and_no_lock(RemoteTestReport* report) {
    RemoteSession session;
    remote_session_initialise(&session);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionLinkDown, session.link_state, "starts down");
    remote_session_port_opened(&session);
    char output[512];
    drain(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strcmp(output, "HELLO version=1 peripheral=stopbath-pico locked=0\n") == 0, output);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionHandshaking, session.link_state, "handshaking");
    RemoteDisplayState display_state;
    remote_session_display(&session, &display_state);
    REMOTE_TEST_ASSERT(report, !display_state.link_connected && display_state.link_connecting, "connecting screen");
}

static void the_first_display_record_is_the_acceptance_and_is_rendered(RemoteTestReport* report) {
    RemoteSession session = connected_session(WIFI_RECORD);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionConnected, session.link_state, "connected");
    RemoteDisplayState display_state;
    remote_session_display(&session, &display_state);
    REMOTE_TEST_ASSERT(report, display_state.link_connected, "link up");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteDisplayStatusPresenting, display_state.status, "status");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteDisplayPageWifi, display_state.page, "page");
    REMOTE_TEST_ASSERT(report, strcmp(display_state.payload, "WIFI:T:WPA;S:fix;P:pass;;") == 0, "payload decoded");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 2, display_state.delivered_count, "delivered");
    REMOTE_TEST_ASSERT(report, display_state.error_code[0] == '\0', "no error");
}

static void a_later_record_replaces_the_previous_one_wholly(RemoteTestReport* report) {
    RemoteSession session = connected_session(WIFI_RECORD);
    feed(&session, READY_RECORD);
    RemoteDisplayState display_state;
    remote_session_display(&session, &display_state);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteDisplayStatusReady, display_state.status, "status replaced");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteDisplayPageNone, display_state.page, "page replaced");
    REMOTE_TEST_ASSERT(report, display_state.payload[0] == '\0', "payload gone, not merged");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, display_state.delivered_count, "count replaced");
}

static void an_error_code_is_shown_as_its_wire_name(RemoteTestReport* report) {
    RemoteSession session = connected_session(READY_RECORD);
    feed(&session, GUARD_ERROR_RECORD);
    RemoteDisplayState display_state;
    remote_session_display(&session, &display_state);
    REMOTE_TEST_ASSERT(report, strcmp(display_state.error_code, "GUARD") == 0, display_state.error_code);
    feed(&session, READY_RECORD);
    remote_session_display(&session, &display_state);
    REMOTE_TEST_ASSERT(report, display_state.error_code[0] == '\0', "cleared by the next record");
}

static void bad_version_marks_the_link_incompatible(RemoteTestReport* report) {
    RemoteSession session;
    remote_session_initialise(&session);
    remote_session_port_opened(&session);
    feed(&session, BAD_VERSION_RECORD);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionIncompatible, session.link_state, "incompatible");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1, session.version_mismatches, "counted");
    RemoteDisplayState display_state;
    remote_session_display(&session, &display_state);
    REMOTE_TEST_ASSERT(report, !display_state.link_connected && display_state.link_incompatible, "incompatible screen");
    REMOTE_TEST_ASSERT(report, !remote_session_report_press(&session, RemoteInputKey0, RemoteInputPressShort), "no press is sent");
}

static void closing_the_port_discards_everything_and_shows_not_connected(RemoteTestReport* report) {
    RemoteSession session = connected_session(WIFI_RECORD);
    remote_session_report_press(&session, RemoteInputKey0, RemoteInputPressShort);
    remote_session_port_closed(&session);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionLinkDown, session.link_state, "down");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1, session.reconnections, "counted as a reconnection");
    RemoteDisplayState display_state;
    remote_session_display(&session, &display_state);
    REMOTE_TEST_ASSERT(report, !display_state.link_connected, "not connected screen");
    REMOTE_TEST_ASSERT(report, display_state.payload[0] == '\0', "the passphrase is gone");
    char output[512];
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, drain(&session, output, sizeof(output)), "the unsent press was dropped, not kept");

    /* Reopening starts a fresh handshake and nothing from before survives. */
    remote_session_port_opened(&session);
    drain(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strncmp(output, "HELLO ", 6) == 0, "only a fresh HELLO");
    REMOTE_TEST_ASSERT(report, strstr(output, "BUTTON") == NULL, "no replayed press");
}

static void a_line_cut_by_a_disconnection_is_not_completed_after_reconnection(RemoteTestReport* report) {
    RemoteSession session = connected_session(READY_RECORD);
    feed(&session, "DISPLAY status=PRESENTING page=WIFI payload=abc del");
    remote_session_port_closed(&session);
    remote_session_port_opened(&session);
    feed(&session, "ivered=5 error=NONE\n");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionHandshaking, session.link_state, "the fragment did not become a record");
    feed(&session, READY_RECORD);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionConnected, session.link_state, "a whole record does");
}

static void a_press_is_sent_only_while_connected(RemoteTestReport* report) {
    RemoteSession session;
    remote_session_initialise(&session);
    REMOTE_TEST_ASSERT(report, !remote_session_report_press(&session, RemoteInputKey0, RemoteInputPressShort), "down: dropped");
    remote_session_port_opened(&session);
    REMOTE_TEST_ASSERT(report, !remote_session_report_press(&session, RemoteInputKey0, RemoteInputPressShort), "handshaking: dropped");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 2, session.events_dropped_no_link, "both counted");
    char output[512];
    drain(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strstr(output, "BUTTON") == NULL, "nothing queued for later");
    feed(&session, READY_RECORD);
    REMOTE_TEST_ASSERT(report, remote_session_report_press(&session, RemoteInputKey0, RemoteInputPressShort), "connected: sent");
}

typedef struct {
    RemoteInputKey key;
    RemoteInputPressKind press_kind;
    const char* record_before;
    const char* expected_line;
    const char* description;
} PressRow;

static const PressRow press_rows[] = {
    {RemoteInputKey0, RemoteInputPressShort, READY_RECORD, "BUTTON event=CENTER_SHORT foregrounded=1 unlocked=1\n", "key0 short is CENTER_SHORT"},
    {RemoteInputKey0, RemoteInputPressLong, WIFI_RECORD, "BUTTON event=CENTER_LONG foregrounded=1 unlocked=1\n", "key0 long is CENTER_LONG"},
    {RemoteInputKey1, RemoteInputPressShort, WIFI_RECORD, "BUTTON event=RIGHT_SHORT foregrounded=1 unlocked=1\n", "key1 on the wifi page asks for the guest page"},
    {RemoteInputKey1, RemoteInputPressShort, GUEST_RECORD, "BUTTON event=LEFT_SHORT foregrounded=1 unlocked=1\n", "key1 on the guest page asks for the wifi page"},
    {RemoteInputKey1, RemoteInputPressShort, READY_RECORD, "BUTTON event=RIGHT_SHORT foregrounded=1 unlocked=1\n", "key1 with no page sends RIGHT_SHORT, which the appliance ignores outside a session"},
};

static void every_press_encodes_its_event_with_both_guard_flags_true(RemoteTestReport* report) {
    for(int row_index = 0; row_index < REMOTE_TEST_ROW_COUNT(press_rows); row_index++) {
        const PressRow* row = &press_rows[row_index];
        RemoteSession session = connected_session(row->record_before);
        REMOTE_TEST_ASSERT(report, remote_session_report_press(&session, row->key, row->press_kind), row->description);
        char output[512];
        drain(&session, output, sizeof(output));
        REMOTE_TEST_ASSERT(report, strcmp(output, row->expected_line) == 0, output);
    }
}

static void key1_long_sends_nothing_and_is_not_an_error(RemoteTestReport* report) {
    RemoteSession session = connected_session(WIFI_RECORD);
    REMOTE_TEST_ASSERT(report, !remote_session_report_press(&session, RemoteInputKey1, RemoteInputPressLong), "reserved, unsent");
    char output[512];
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, drain(&session, output, sizeof(output)), "nothing on the wire");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, session.events_dropped_no_link, "not counted as a drop");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, session.events_dropped_by_output_full, "nor as a full queue");
}

/* The one function of spec 2.3: whatever the page value, including values
 * outside the enumeration, Key1 can only ever produce one of the two page
 * events, never anything destructive. */
static void the_key1_mapping_can_only_produce_a_page_event(RemoteTestReport* report) {
    for(int page_value = -8; page_value < 16; page_value++) {
        RemoteProtocolEvent event = remote_session_page_event_for_key1(page_value);
        REMOTE_TEST_ASSERT(
            report, event == RemoteProtocolEventLeftShort || event == RemoteProtocolEventRightShort,
            "only LEFT_SHORT or RIGHT_SHORT");
        if(page_value == (int)RemoteProtocolPageGuest) {
            REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteProtocolEventLeftShort, event, "guest page goes left");
        } else {
            REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteProtocolEventRightShort, event, "everything else goes right");
        }
    }
}

static void the_output_queue_is_bounded_and_an_overflowing_press_is_dropped_and_counted(RemoteTestReport* report) {
    RemoteSession session = connected_session(READY_RECORD);
    int sent = 0;
    for(int press = 0; press < REMOTE_SESSION_OUTBOUND_QUEUE_DEPTH + 2; press++) {
        if(remote_session_report_press(&session, RemoteInputKey0, RemoteInputPressShort)) {
            sent++;
        }
    }
    REMOTE_TEST_ASSERT_EQUAL_INT(report, REMOTE_SESSION_OUTBOUND_QUEUE_DEPTH, sent, "the depth is the bound");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 2, session.events_dropped_by_output_full, "the rest were dropped and counted");
    char output[REMOTE_PROTOCOL_MAXIMUM_MESSAGE_LENGTH * (REMOTE_SESSION_OUTBOUND_QUEUE_DEPTH + 1)];
    drain(&session, output, sizeof(output));
    int lines = 0;
    for(const char* cursor = output; *cursor != '\0'; cursor++) {
        if(*cursor == '\n') {
            lines++;
        }
    }
    REMOTE_TEST_ASSERT_EQUAL_INT(report, REMOTE_SESSION_OUTBOUND_QUEUE_DEPTH, lines, "exactly that many lines drained");
    REMOTE_TEST_ASSERT(report, remote_session_report_press(&session, RemoteInputKey0, RemoteInputPressShort), "room again after draining");
}

static void a_lost_handshake_is_retried_after_the_interval(RemoteTestReport* report) {
    RemoteSession session;
    remote_session_initialise(&session);
    remote_session_port_opened(&session);
    char output[512];
    drain(&session, output, sizeof(output));
    remote_session_tick(&session, REMOTE_SESSION_HANDSHAKE_RETRY_INTERVAL_MILLISECONDS - 1);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, drain(&session, output, sizeof(output)), "nothing before the interval");
    remote_session_tick(&session, 1);
    drain(&session, output, sizeof(output));
    REMOTE_TEST_ASSERT(report, strncmp(output, "HELLO ", 6) == 0, "HELLO again at the interval");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1, session.handshake_retries, "counted");
    feed(&session, READY_RECORD);
    remote_session_tick(&session, REMOTE_SESSION_HANDSHAKE_RETRY_INTERVAL_MILLISECONDS * 3);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, drain(&session, output, sizeof(output)), "no retry once connected");
}

static void malformed_input_is_counted_and_does_not_disturb_the_record(RemoteTestReport* report) {
    RemoteSession session = connected_session(WIFI_RECORD);
    feed(&session, "NONSENSE\n");
    feed(&session, "BUTTON event=CENTER_LONG foregrounded=1 unlocked=1\n");
    feed(&session, "DISPLAY status=READY page=NONE payload=\x01 delivered=0 error=NONE\n");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 3, session.malformed_received, "three refusals counted");
    RemoteDisplayState display_state;
    remote_session_display(&session, &display_state);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteDisplayPageWifi, display_state.page, "still showing the last good record");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteSessionConnected, session.link_state, "still connected");
}

static void the_display_carries_the_session_counters_for_diagnostics(RemoteTestReport* report) {
    RemoteSession session;
    remote_session_initialise(&session);
    remote_session_port_opened(&session);
    feed(&session, BAD_VERSION_RECORD);
    remote_session_port_closed(&session);
    RemoteDisplayState display_state;
    remote_session_display(&session, &display_state);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1, display_state.diagnostics.reconnections, "reconnections");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1, display_state.diagnostics.version_mismatches, "version mismatches");
}

static void the_session_never_allocates(RemoteTestReport* report) {
    int allocations_before = remote_test_allocation_count;
    RemoteSession session = connected_session(WIFI_RECORD);
    remote_session_report_press(&session, RemoteInputKey1, RemoteInputPressShort);
    char output[512];
    drain(&session, output, sizeof(output));
    feed(&session, GUEST_RECORD);
    RemoteDisplayState display_state;
    remote_session_display(&session, &display_state);
    remote_session_port_closed(&session);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, allocations_before, remote_test_allocation_count, "no allocation");
}

/* Owed by KE2: the error band across every code the protocol defines. Each
 * arrives on the wire, is shown as its own name, and stays inside the band
 * (every code is at most eleven characters, the band's width). */
static void every_error_code_is_shown_by_name_inside_the_error_band(RemoteTestReport* report) {
    int error_count = 0;
    const char* const* error_names = remote_protocol_enumeration_values(RemoteProtocolEnumerationError, &error_count);
    REMOTE_TEST_ASSERT(report, error_count > 10, "the code set is present");
    RemoteSession plain = connected_session(READY_RECORD);
    RemoteDisplayState plain_state;
    remote_session_display(&plain, &plain_state);
    RemoteBitmap plain_bitmap;
    remote_display_layout_render(&plain_state, &plain_bitmap);
    RemoteLayoutRectangle band = remote_display_layout_region(RemoteLayoutRegionError);
    for(int error = 0; error < error_count; error++) {
        if(error == (int)RemoteProtocolErrorNone || error == (int)RemoteProtocolErrorBadVersion) {
            continue;
        }
        char line[160] = "DISPLAY status=READY page=NONE payload= delivered=0 error=";
        strncat(line, error_names[error], sizeof(line) - strlen(line) - 2);
        strncat(line, "\n", sizeof(line) - strlen(line) - 1);
        RemoteSession session = connected_session(READY_RECORD);
        feed(&session, line);
        RemoteDisplayState display_state;
        remote_session_display(&session, &display_state);
        REMOTE_TEST_ASSERT(report, strcmp(display_state.error_code, error_names[error]) == 0, error_names[error]);
        REMOTE_TEST_ASSERT(report, strlen(display_state.error_code) <= 11, "fits the band");
        RemoteBitmap bitmap;
        remote_display_layout_render(&display_state, &bitmap);
        for(int y = 0; y < REMOTE_BITMAP_HEIGHT; y++) {
            for(int x = 0; x < REMOTE_BITMAP_WIDTH; x++) {
                bool inside_band = x >= band.x && x < band.x + band.width && y >= band.y && y < band.y + band.height;
                if(!inside_band && remote_bitmap_get_pixel(&bitmap, x, y) != remote_bitmap_get_pixel(&plain_bitmap, x, y)) {
                    REMOTE_TEST_ASSERT(report, false, error_names[error]);
                    y = REMOTE_BITMAP_HEIGHT;
                    break;
                }
            }
        }
    }
}

static const RemoteTestCase test_cases[] = {
    {"opening the port sends hello with the fixed token and no lock", opening_the_port_sends_hello_with_the_fixed_token_and_no_lock},
    {"the first display record is the acceptance and is rendered", the_first_display_record_is_the_acceptance_and_is_rendered},
    {"a later record replaces the previous one wholly", a_later_record_replaces_the_previous_one_wholly},
    {"an error code is shown as its wire name", an_error_code_is_shown_as_its_wire_name},
    {"bad version marks the link incompatible", bad_version_marks_the_link_incompatible},
    {"closing the port discards everything and shows not connected", closing_the_port_discards_everything_and_shows_not_connected},
    {"a line cut by a disconnection is not completed after reconnection", a_line_cut_by_a_disconnection_is_not_completed_after_reconnection},
    {"a press is sent only while connected", a_press_is_sent_only_while_connected},
    {"every press encodes its event with both guard flags true", every_press_encodes_its_event_with_both_guard_flags_true},
    {"key1 long sends nothing and is not an error", key1_long_sends_nothing_and_is_not_an_error},
    {"the key1 mapping can only produce a page event", the_key1_mapping_can_only_produce_a_page_event},
    {"the output queue is bounded and an overflowing press is dropped and counted",
     the_output_queue_is_bounded_and_an_overflowing_press_is_dropped_and_counted},
    {"a lost handshake is retried after the interval", a_lost_handshake_is_retried_after_the_interval},
    {"malformed input is counted and does not disturb the record", malformed_input_is_counted_and_does_not_disturb_the_record},
    {"the display carries the session counters for diagnostics", the_display_carries_the_session_counters_for_diagnostics},
    {"the session never allocates", the_session_never_allocates},
    {"every error code is shown by name inside the error band", every_error_code_is_shown_by_name_inside_the_error_band},
};

int main(void) {
    return remote_test_run_all(test_cases, REMOTE_TEST_ROW_COUNT(test_cases));
}
