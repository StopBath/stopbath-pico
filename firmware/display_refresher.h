/*
 * Drives the panel from display states without ever waiting on it.
 *
 * Holds what the panel was last told to show and what it should show next,
 * asks the refresh policy what the change costs, renders, begins the
 * refresh, and returns; the caller polls service() every tick, which
 * notices a finished refresh and begins the next one due. Shared by the
 * KE2 layout demo and the remote itself (Flipper 0.7), so the refresh
 * behaviour the author judged on the glass is the behaviour the product has.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "../remote_display/remote_bitmap.h"
#include "../remote_display/remote_display_layout.h"
#include "../remote_display/remote_refresh_policy.h"

typedef void (*DisplayRefresherOverlay)(const RemoteDisplayState* state, void* context);

typedef struct {
    RemoteDisplayState shown;
    RemoteDisplayState target;
    RemoteRefreshPolicy policy;
    unsigned pending_regions;
    bool refresh_in_progress;
    uint64_t refresh_started_microseconds;
    /* The refresh counters the diagnostic screen shows. */
    uint32_t full_refreshes;
    uint32_t partial_refreshes;
    uint32_t last_refresh_milliseconds;
    DisplayRefresherOverlay overlay;
    void* overlay_context;
} DisplayRefresher;

/* The frame every refresh is drawn into, owned here and sized once. */
extern RemoteBitmap display_refresher_frame;

/* Draws the first state with a blocking full refresh, so the glass and the
 * record agree from the start; nothing else is happening yet. The overlay
 * is an optional hook for a program to draw over the rendered frame before
 * it is sent, in the strip no region claims; NULL for none. */
void display_refresher_initialise(
    DisplayRefresher* refresher,
    const RemoteDisplayState* first_state,
    DisplayRefresherOverlay overlay,
    void* overlay_context);

/* What the panel should show next. May be called while a refresh runs; the
 * change is picked up when the panel is free. */
void display_refresher_set_target(DisplayRefresher* refresher, const RemoteDisplayState* target);

/* Called every tick. Never blocks. Returns true while a refresh is running. */
bool display_refresher_service(DisplayRefresher* refresher);

