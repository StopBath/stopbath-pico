/*
 * The display layout: what goes where on the 400 by 300 panel for a given
 * display state, and which region each field lives in.
 *
 * No SDK dependency, so every screen is composed and checked on the
 * development machine from a fixture (Pico spec KE2). The firmware hands the
 * composed bitmap to the panel and decides nothing about placement.
 *
 * The state is the one record the appliance sends (extension 3.2: status,
 * page, payload, delivered count, error code) plus the facts only the device
 * knows: the link state and whether the diagnostic screen is up. The device
 * renders the last record received and holds no belief of its own about any
 * of it (Flipper 2.5). The status is rendered as a short label this module
 * owns and the error code verbatim, as the Flipper does: the appliance sends
 * codes, never sentences, and presentation is the peripheral's.
 *
 * Regions exist because the panel is e-paper (spec 2.6). Each field is
 * drawn inside exactly one region, so a change to that field can be
 * refreshed in place without touching the code a guest may be scanning. The
 * regions are byte aligned in x because the controller addresses RAM eight
 * pixels at a time. Their positions are the KD8 proposal, accepted or amended
 * against the real panel at the KE2 gate, and are expressed once, here.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "remote_bitmap.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Status codes fixed by extension 3.2. Their wire spelling is the protocol
 * library's (KE3); the order here is not a contract. */
typedef enum {
    RemoteDisplayStatusReady,
    RemoteDisplayStatusPresenting,
    RemoteDisplayStatusGuestConnected,
    RemoteDisplayStatusTerminating,
    RemoteDisplayStatusRecoveryRequired,
    RemoteDisplayStatusCount,
} RemoteDisplayStatus;

typedef enum {
    RemoteDisplayPageNone,
    RemoteDisplayPageWifi,
    RemoteDisplayPageGuest,
    RemoteDisplayPageCount,
} RemoteDisplayPage;

/* The protocol's payload bound plus a terminator, and room for the longest
 * bounded code plus a terminator. Sized once so nothing allocates in the
 * input path (Flipper 0.10). */
#define REMOTE_DISPLAY_PAYLOAD_CAPACITY 257
#define REMOTE_DISPLAY_CODE_CAPACITY 33

/* Counters the diagnostic screen shows. The link ones are filled by the
 * session (KE3); the refresh ones by the firmware's panel driving. */
typedef struct {
    uint32_t reconnections;
    uint32_t malformed_received;
    uint32_t version_mismatches;
    uint32_t events_dropped_no_link;
    uint32_t events_dropped_by_output_full;
    uint32_t handshake_retries;
    uint32_t full_refreshes;
    uint32_t partial_refreshes;
    uint32_t last_refresh_milliseconds;
} RemoteDisplayDiagnostics;

typedef struct {
    /* Plain integers rather than the enums so an out of range value received
     * from the appliance renders as a safe fallback rather than invoking
     * undefined behaviour. */
    int status;
    int page;
    char payload[REMOTE_DISPLAY_PAYLOAD_CAPACITY];
    unsigned int delivered_count;
    /* Empty when there is no error. Rendered as the code itself, so nothing
     * here is kept in step with the code set the appliance defines. */
    char error_code[REMOTE_DISPLAY_CODE_CAPACITY];
    /* Local link facts, shown instead of the appliance's record when the
     * link is not connected. Incompatible wins over connecting, which wins
     * over a plain disconnection. */
    bool link_connected;
    bool link_connecting;
    bool link_incompatible;
    /* The diagnostic screen replaces everything while it is up. */
    bool show_diagnostics;
    RemoteDisplayDiagnostics diagnostics;
} RemoteDisplayState;

/* Sets every field to the not connected, empty state. */
void remote_display_state_initialise(RemoteDisplayState* display_state);

/* The regions. Every pixel the layout draws for a connected, non diagnostic
 * state lies inside exactly one of them; the rest of the panel stays white. */
typedef enum {
    /* Title and status label, the full width of the top strip. */
    RemoteLayoutRegionHeader,
    /* The code square. Changes only with page or payload (spec 2.6). */
    RemoteLayoutRegionCode,
    /* Which page this is, in the column beside the code. */
    RemoteLayoutRegionPage,
    /* What the keys do now, which depends on status and page. */
    RemoteLayoutRegionHint,
    /* The delivered count. */
    RemoteLayoutRegionDelivered,
    /* The error band. */
    RemoteLayoutRegionError,
    RemoteLayoutRegionCount,
} RemoteLayoutRegion;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} RemoteLayoutRectangle;

RemoteLayoutRectangle remote_display_layout_region(RemoteLayoutRegion region);

/* Composes the whole panel for the state into the bitmap. Clears it first. */
void remote_display_layout_render(const RemoteDisplayState* display_state, RemoteBitmap* bitmap);

/* Which regions differ between two states, as a mask of 1 << region,
 * computed from the fields rather than the pixels so the rule that a field
 * lives in one region is stated once. A change of link state or of the
 * diagnostic screen reports every region, because those screens replace the
 * whole panel. */
unsigned remote_display_layout_changed_regions(const RemoteDisplayState* before, const RemoteDisplayState* after);

/* The side of the square the code is drawn in, inside the code region: the
 * Appendix A starting point of a version 3 code at seven pixels a module
 * with a four module quiet zone, settled by scanning at the KE5 gate. */
#define REMOTE_LAYOUT_CODE_SIDE 259

#ifdef __cplusplus
}
#endif
