#include "remote_display_fixtures.h"

#include <string.h>

#define FIXTURE_CAPACITY 24

static RemoteDisplayFixture fixtures[FIXTURE_CAPACITY];
static int fixture_count = 0;
static bool fixtures_built = false;

static RemoteDisplayState base_state(bool connected, int status, int page, const char* payload) {
    RemoteDisplayState display_state;
    remote_display_state_initialise(&display_state);
    display_state.link_connected = connected;
    display_state.status = status;
    display_state.page = page;
    if(payload != NULL) {
        strncpy(display_state.payload, payload, REMOTE_DISPLAY_PAYLOAD_CAPACITY - 1);
    }
    return display_state;
}

static void add_fixture(const char* name, RemoteDisplayState display_state) {
    if(fixture_count >= FIXTURE_CAPACITY) {
        return;
    }
    fixtures[fixture_count].fixture_name = name;
    fixtures[fixture_count].display_state = display_state;
    fixture_count++;
}

/* Built on first use rather than as a static initialiser, because the state
 * carries arrays a designated initialiser would leave hard to read. Nothing
 * allocates: the table is static storage sized once. */
static void build_fixtures(void) {
    RemoteDisplayState disconnected = base_state(false, RemoteDisplayStatusReady, RemoteDisplayPageNone, NULL);
    add_fixture("not connected", disconnected);

    RemoteDisplayState connecting = disconnected;
    connecting.link_connecting = true;
    add_fixture("connecting", connecting);

    RemoteDisplayState incompatible = disconnected;
    incompatible.link_incompatible = true;
    add_fixture("incompatible", incompatible);

    add_fixture("ready", base_state(true, RemoteDisplayStatusReady, RemoteDisplayPageNone, NULL));

    RemoteDisplayState presenting_wifi =
        base_state(true, RemoteDisplayStatusPresenting, RemoteDisplayPageWifi, REMOTE_DISPLAY_FIXTURE_WIFI_PAYLOAD);
    add_fixture("presenting wifi", presenting_wifi);

    RemoteDisplayState presenting_guest =
        base_state(true, RemoteDisplayStatusPresenting, RemoteDisplayPageGuest, REMOTE_DISPLAY_FIXTURE_GUEST_PAYLOAD);
    add_fixture("presenting guest", presenting_guest);

    RemoteDisplayState guest_connected_wifi = presenting_wifi;
    guest_connected_wifi.status = RemoteDisplayStatusGuestConnected;
    add_fixture("guest connected wifi", guest_connected_wifi);

    RemoteDisplayState guest_connected_guest = presenting_guest;
    guest_connected_guest.status = RemoteDisplayStatusGuestConnected;
    guest_connected_guest.delivered_count = 3;
    add_fixture("guest connected guest three delivered", guest_connected_guest);

    add_fixture("terminating", base_state(true, RemoteDisplayStatusTerminating, RemoteDisplayPageNone, NULL));
    add_fixture("recovery required", base_state(true, RemoteDisplayStatusRecoveryRequired, RemoteDisplayPageNone, NULL));

    RemoteDisplayState ready_with_error = base_state(true, RemoteDisplayStatusReady, RemoteDisplayPageNone, NULL);
    strncpy(ready_with_error.error_code, REMOTE_DISPLAY_FIXTURE_ERROR_CODE, REMOTE_DISPLAY_CODE_CAPACITY - 1);
    add_fixture("ready with error", ready_with_error);

    RemoteDisplayState wifi_with_error = guest_connected_wifi;
    strncpy(wifi_with_error.error_code, REMOTE_DISPLAY_FIXTURE_ERROR_CODE, REMOTE_DISPLAY_CODE_CAPACITY - 1);
    add_fixture("guest connected wifi with error", wifi_with_error);

    RemoteDisplayState short_error = guest_connected_wifi;
    strncpy(short_error.error_code, "GUARD", REMOTE_DISPLAY_CODE_CAPACITY - 1);
    add_fixture("guest connected wifi with a short error", short_error);

    add_fixture("unknown status", base_state(true, REMOTE_DISPLAY_FIXTURE_UNKNOWN_STATUS, RemoteDisplayPageNone, NULL));

    const unsigned int counts[] = {0, 9, 999, 9999};
    const char* count_names[] = {"none delivered", "nine delivered", "999 delivered", "9999 delivered"};
    for(int count_index = 0; count_index < 4; count_index++) {
        RemoteDisplayState counted = guest_connected_wifi;
        counted.delivered_count = counts[count_index];
        add_fixture(count_names[count_index], counted);
    }

    RemoteDisplayState diagnostics =
        base_state(true, RemoteDisplayStatusPresenting, RemoteDisplayPageWifi, REMOTE_DISPLAY_FIXTURE_WIFI_PAYLOAD);
    diagnostics.show_diagnostics = true;
    diagnostics.diagnostics.reconnections = 3;
    diagnostics.diagnostics.malformed_received = 1;
    diagnostics.diagnostics.full_refreshes = 12;
    diagnostics.diagnostics.partial_refreshes = 47;
    diagnostics.diagnostics.last_refresh_milliseconds = 3921;
    add_fixture("diagnostics", diagnostics);

    fixtures_built = true;
}

const RemoteDisplayFixture* remote_display_fixtures(void) {
    if(!fixtures_built) {
        build_fixtures();
    }
    return fixtures;
}

int remote_display_fixture_count(void) {
    if(!fixtures_built) {
        build_fixtures();
    }
    return fixture_count;
}
