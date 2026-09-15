/*
 * The refresh policy: what a change to the display state costs the panel.
 *
 * E-paper makes "publish on every change" (extension 3.2) expensive: a full
 * refresh takes seconds and flickers, and a guest part way through scanning
 * would lose the code. So (Pico spec 2.6) only the region whose field
 * changed is refreshed, in place, and a full refresh is reserved for a
 * change to the code itself (page or payload), a change of link screen, and
 * a periodic pass to clear the residue partial refreshes leave behind.
 *
 * A status change alone is partial: PRESENTING becomes GUEST_CONNECTED as
 * the first guest joins, which is exactly when others are still scanning.
 *
 * No SDK dependency; the firmware asks this what to do and drives the panel
 * accordingly.
 */
#pragma once

#include "remote_display_layout.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Partial refreshes before a full one is forced. The manufacturer's FAQ
 * advises a full refresh after five partials to clear residue (evaluation
 * log 4.3); Appendix A lists it as a starting point for the KE2 gate. */
#define REMOTE_REFRESH_PARTIALS_BEFORE_FORCED_FULL 5

typedef enum {
    RemoteRefreshKindNone,
    RemoteRefreshKindPartial,
    RemoteRefreshKindFull,
} RemoteRefreshKind;

typedef struct {
    RemoteRefreshKind kind;
    /* For a partial refresh, the regions to send, as a mask of
     * 1 << RemoteLayoutRegion. Zero otherwise. */
    unsigned regions;
} RemoteRefreshDecision;

typedef struct {
    int partials_since_full;
} RemoteRefreshPolicy;

void remote_refresh_policy_initialise(RemoteRefreshPolicy* policy);

/* What moving from one state to the next costs. Counts the partial it
 * returns; resets the count on a full. */
RemoteRefreshDecision remote_refresh_policy_decide(
    RemoteRefreshPolicy* policy,
    const RemoteDisplayState* before,
    const RemoteDisplayState* after);

/* A full refresh regardless of change: the first draw after connecting, or
 * a manual ghost clearing pass. Resets the count. */
RemoteRefreshDecision remote_refresh_policy_force_full(RemoteRefreshPolicy* policy);

int remote_refresh_policy_partials_since_full(const RemoteRefreshPolicy* policy);

#ifdef __cplusplus
}
#endif
