#include "display_refresher.h"

#include <string.h>

#include "hardware/timer.h"

#include "panel.h"

RemoteBitmap display_refresher_frame;

static void render_target(DisplayRefresher* refresher) {
    remote_display_layout_render(&refresher->target, &display_refresher_frame);
    if(refresher->overlay != NULL) {
        refresher->overlay(&refresher->target, refresher->overlay_context);
    }
    refresher->shown = refresher->target;
}

static void begin(DisplayRefresher* refresher, RemoteRefreshKind kind) {
    refresher->refresh_in_progress = true;
    refresher->refresh_started_microseconds = time_us_64();
    if(kind == RemoteRefreshKindFull) {
        refresher->full_refreshes++;
    } else {
        refresher->partial_refreshes++;
    }
}

static void begin_next_partial_region(DisplayRefresher* refresher) {
    for(int region = 0; region < RemoteLayoutRegionCount; region++) {
        unsigned bit = 1u << region;
        if(refresher->pending_regions & bit) {
            refresher->pending_regions &= ~bit;
            RemoteLayoutRectangle area = remote_display_layout_region((RemoteLayoutRegion)region);
            panel_begin_partial_refresh(display_refresher_frame.bytes, area.x, area.y, area.width, area.height);
            begin(refresher, RemoteRefreshKindPartial);
            return;
        }
    }
}

void display_refresher_initialise(
    DisplayRefresher* refresher,
    const RemoteDisplayState* first_state,
    DisplayRefresherOverlay overlay,
    void* overlay_context) {
    memset(refresher, 0, sizeof(*refresher));
    refresher->overlay = overlay;
    refresher->overlay_context = overlay_context;
    remote_refresh_policy_initialise(&refresher->policy);
    refresher->target = *first_state;
    render_target(refresher);
    remote_refresh_policy_force_full(&refresher->policy);
    uint64_t started = time_us_64();
    panel_show_full_frame(display_refresher_frame.bytes);
    refresher->full_refreshes = 1;
    refresher->last_refresh_milliseconds = (uint32_t)((time_us_64() - started) / 1000u);
}

void display_refresher_set_target(DisplayRefresher* refresher, const RemoteDisplayState* target) {
    refresher->target = *target;
}

bool display_refresher_service(DisplayRefresher* refresher) {
    if(refresher->refresh_in_progress) {
        if(panel_is_busy()) {
            return true;
        }
        refresher->refresh_in_progress = false;
        uint64_t elapsed = time_us_64() - refresher->refresh_started_microseconds;
        refresher->last_refresh_milliseconds = (uint32_t)(elapsed / 1000u);
    }
    if(refresher->pending_regions != 0u) {
        begin_next_partial_region(refresher);
        return true;
    }
    RemoteRefreshDecision decision = remote_refresh_policy_decide(&refresher->policy, &refresher->shown, &refresher->target);
    if(decision.kind == RemoteRefreshKindNone) {
        return false;
    }
    render_target(refresher);
    if(decision.kind == RemoteRefreshKindFull) {
        panel_begin_full_refresh(display_refresher_frame.bytes);
        begin(refresher, RemoteRefreshKindFull);
        return true;
    }
    refresher->pending_regions = decision.regions;
    begin_next_partial_region(refresher);
    return true;
}
