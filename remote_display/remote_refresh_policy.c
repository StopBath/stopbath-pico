#include "remote_refresh_policy.h"

void remote_refresh_policy_initialise(RemoteRefreshPolicy* policy) {
    policy->partials_since_full = 0;
}

static RemoteRefreshDecision full(RemoteRefreshPolicy* policy) {
    policy->partials_since_full = 0;
    RemoteRefreshDecision decision = {.kind = RemoteRefreshKindFull, .regions = 0u};
    return decision;
}

RemoteRefreshDecision remote_refresh_policy_decide(
    RemoteRefreshPolicy* policy,
    const RemoteDisplayState* before,
    const RemoteDisplayState* after) {
    unsigned changed = remote_display_layout_changed_regions(before, after);
    if(changed == 0u) {
        RemoteRefreshDecision none = {.kind = RemoteRefreshKindNone, .regions = 0u};
        return none;
    }
    /* The code region is the guest's; the whole panel is redrawn for it
     * rather than a partial pass that would leave the old code ghosted
     * under the new one. A link screen change reports every region and
     * lands here too. */
    if(changed & (1u << RemoteLayoutRegionCode)) {
        return full(policy);
    }
    if(policy->partials_since_full + 1 >= REMOTE_REFRESH_PARTIALS_BEFORE_FORCED_FULL) {
        return full(policy);
    }
    policy->partials_since_full++;
    RemoteRefreshDecision partial = {.kind = RemoteRefreshKindPartial, .regions = changed};
    return partial;
}

RemoteRefreshDecision remote_refresh_policy_force_full(RemoteRefreshPolicy* policy) {
    return full(policy);
}

int remote_refresh_policy_partials_since_full(const RemoteRefreshPolicy* policy) {
    return policy->partials_since_full;
}
