/*
 * One fixture per display state worth looking at. Shared by the host tests
 * and by the KE2 firmware, which cycles through them on the panel so the
 * author can judge each one at arm's length (Pico spec KE2, hardware gate).
 * Shared rather than duplicated (Flipper 0.7).
 *
 * Any code or payload in here is fixture text, not a wire value. The status
 * and error code sets are the protocol's, which arrives in KE3; the sweep of
 * the error band across every real code is a KE3 test.
 */
#pragma once

#include "remote_display_layout.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* fixture_name;
    RemoteDisplayState display_state;
} RemoteDisplayFixture;

/* Sentinel for the unknown status fixture: outside the enumeration on
 * purpose, so the fallback path is exercised. */
#define REMOTE_DISPLAY_FIXTURE_UNKNOWN_STATUS (RemoteDisplayStatusCount + 3)

/* Fixture text, not wire values. The Wi-Fi payload is 53 bytes, the length
 * the appliance's real payload measured on the Flipper, so the code region
 * shows the figure that matters. */
#define REMOTE_DISPLAY_FIXTURE_WIFI_PAYLOAD "WIFI:T:WPA;S:StopBath-fixture;P:fixture-passphrase-1;;"
#define REMOTE_DISPLAY_FIXTURE_GUEST_PAYLOAD "HTTP://192.168.72.1/"
/* Longer than the eleven cells the band holds, so truncation is visible. */
#define REMOTE_DISPLAY_FIXTURE_ERROR_CODE "ERROR_FIXTURE"

const RemoteDisplayFixture* remote_display_fixtures(void);
int remote_display_fixture_count(void);

#ifdef __cplusplus
}
#endif
