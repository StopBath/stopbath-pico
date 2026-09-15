/*
 * KE2 tests first (Pico spec, KE2): which changes cost a partial refresh,
 * which a full one, and the forced full refresh after too many partials.
 */
#include "test_support.h"

#include "../remote_display/remote_display_layout.h"
#include "../remote_display/remote_refresh_policy.h"

static RemoteDisplayState presenting_wifi(void) {
    RemoteDisplayState display_state;
    remote_display_state_initialise(&display_state);
    display_state.link_connected = true;
    display_state.status = RemoteDisplayStatusPresenting;
    display_state.page = RemoteDisplayPageWifi;
    strncpy(display_state.payload, "WIFI:T:WPA;S:fixture;P:fixturepass;;", REMOTE_DISPLAY_PAYLOAD_CAPACITY - 1);
    return display_state;
}

static void no_change_is_no_refresh(RemoteTestReport* report) {
    RemoteRefreshPolicy policy;
    remote_refresh_policy_initialise(&policy);
    RemoteDisplayState state = presenting_wifi();
    RemoteRefreshDecision decision = remote_refresh_policy_decide(&policy, &state, &state);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteRefreshKindNone, decision.kind, "nothing to do");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, decision.regions, "no regions");
}

/* The count and the error band are the fields that change while a guest is
 * scanning; they are refreshed in place and the code is left alone. */
static void a_count_or_error_change_is_a_partial_refresh_of_that_region(RemoteTestReport* report) {
    RemoteRefreshPolicy policy;
    remote_refresh_policy_initialise(&policy);
    RemoteDisplayState before = presenting_wifi();
    RemoteDisplayState after = before;
    after.delivered_count = 1;
    RemoteRefreshDecision decision = remote_refresh_policy_decide(&policy, &before, &after);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteRefreshKindPartial, decision.kind, "count is partial");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1u << RemoteLayoutRegionDelivered, decision.regions, "count region only");

    RemoteDisplayState with_error = after;
    strncpy(with_error.error_code, "GUARD", REMOTE_DISPLAY_CODE_CAPACITY - 1);
    decision = remote_refresh_policy_decide(&policy, &after, &with_error);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteRefreshKindPartial, decision.kind, "error is partial");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1u << RemoteLayoutRegionError, decision.regions, "error region only");

    /* A status change while the page stays: PRESENTING to GUEST_CONNECTED
     * happens as the first guest joins, while others may still be scanning,
     * so it must not be a full refresh either. */
    RemoteDisplayState guest_joined = with_error;
    guest_joined.status = RemoteDisplayStatusGuestConnected;
    decision = remote_refresh_policy_decide(&policy, &with_error, &guest_joined);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteRefreshKindPartial, decision.kind, "status is partial");
    REMOTE_TEST_ASSERT(report, (decision.regions & (1u << RemoteLayoutRegionCode)) == 0, "and never the code");
}

static void a_page_or_payload_or_link_change_is_a_full_refresh(RemoteTestReport* report) {
    RemoteRefreshPolicy policy;
    remote_refresh_policy_initialise(&policy);
    RemoteDisplayState before = presenting_wifi();

    RemoteDisplayState guest_page = before;
    guest_page.page = RemoteDisplayPageGuest;
    strncpy(guest_page.payload, "HTTP://192.168.72.1/", REMOTE_DISPLAY_PAYLOAD_CAPACITY - 1);
    REMOTE_TEST_ASSERT_EQUAL_INT(
        report, RemoteRefreshKindFull, remote_refresh_policy_decide(&policy, &before, &guest_page).kind, "page change");

    RemoteDisplayState new_payload = before;
    strncpy(new_payload.payload, "WIFI:T:WPA;S:other;P:otherpass;;", REMOTE_DISPLAY_PAYLOAD_CAPACITY - 1);
    REMOTE_TEST_ASSERT_EQUAL_INT(
        report, RemoteRefreshKindFull, remote_refresh_policy_decide(&policy, &before, &new_payload).kind, "payload change");

    RemoteDisplayState dropped = before;
    dropped.link_connected = false;
    REMOTE_TEST_ASSERT_EQUAL_INT(
        report, RemoteRefreshKindFull, remote_refresh_policy_decide(&policy, &before, &dropped).kind, "link drop");

    RemoteDisplayState ready = before;
    ready.status = RemoteDisplayStatusReady;
    ready.page = RemoteDisplayPageNone;
    ready.payload[0] = '\0';
    REMOTE_TEST_ASSERT_EQUAL_INT(
        report, RemoteRefreshKindFull, remote_refresh_policy_decide(&policy, &before, &ready).kind, "session ended");
}

/* The manufacturer advises a full refresh after several partial ones to
 * clear residue. The policy counts partials and forces a full refresh at
 * the bound; any full refresh resets the count. */
static void the_forced_full_refresh_arrives_at_the_bound_and_any_full_refresh_resets_it(RemoteTestReport* report) {
    RemoteRefreshPolicy policy;
    remote_refresh_policy_initialise(&policy);
    RemoteDisplayState state = presenting_wifi();
    for(int partial = 1; partial < REMOTE_REFRESH_PARTIALS_BEFORE_FORCED_FULL; partial++) {
        RemoteDisplayState next = state;
        next.delivered_count = state.delivered_count + 1;
        RemoteRefreshDecision decision = remote_refresh_policy_decide(&policy, &state, &next);
        REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteRefreshKindPartial, decision.kind, "still partial under the bound");
        state = next;
    }
    RemoteDisplayState at_bound = state;
    at_bound.delivered_count = state.delivered_count + 1;
    RemoteRefreshDecision forced = remote_refresh_policy_decide(&policy, &state, &at_bound);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteRefreshKindFull, forced.kind, "the bound forces a full refresh");
    state = at_bound;

    RemoteDisplayState after_reset = state;
    after_reset.delivered_count = state.delivered_count + 1;
    REMOTE_TEST_ASSERT_EQUAL_INT(
        report, RemoteRefreshKindPartial, remote_refresh_policy_decide(&policy, &state, &after_reset).kind,
        "the count starts again after the forced full");
    state = after_reset;

    /* Two partials, then a page change (full), then the count is fresh. */
    for(int partial = 0; partial < 2; partial++) {
        RemoteDisplayState next = state;
        next.delivered_count = state.delivered_count + 1;
        remote_refresh_policy_decide(&policy, &state, &next);
        state = next;
    }
    RemoteDisplayState guest_page = state;
    guest_page.page = RemoteDisplayPageGuest;
    strncpy(guest_page.payload, "HTTP://192.168.72.1/", REMOTE_DISPLAY_PAYLOAD_CAPACITY - 1);
    remote_refresh_policy_decide(&policy, &state, &guest_page);
    state = guest_page;
    for(int partial = 1; partial < REMOTE_REFRESH_PARTIALS_BEFORE_FORCED_FULL; partial++) {
        RemoteDisplayState next = state;
        next.delivered_count = state.delivered_count + 1;
        REMOTE_TEST_ASSERT_EQUAL_INT(
            report, RemoteRefreshKindPartial, remote_refresh_policy_decide(&policy, &state, &next).kind,
            "a full refresh for any reason reset the count");
        state = next;
    }
}

/* The policy may be asked to force a full refresh regardless of change, for
 * the first draw after connecting and for a manual ghost clearing pass. */
static void a_forced_full_refresh_is_always_full_and_resets_the_count(RemoteTestReport* report) {
    RemoteRefreshPolicy policy;
    remote_refresh_policy_initialise(&policy);
    RemoteDisplayState state = presenting_wifi();
    RemoteRefreshDecision decision = remote_refresh_policy_force_full(&policy);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteRefreshKindFull, decision.kind, "forced");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 0, remote_refresh_policy_partials_since_full(&policy), "count reset");
    RemoteDisplayState next = state;
    next.delivered_count = 1;
    remote_refresh_policy_decide(&policy, &state, &next);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, 1, remote_refresh_policy_partials_since_full(&policy), "one partial since");
}

static const RemoteTestCase test_cases[] = {
    {"no change is no refresh", no_change_is_no_refresh},
    {"a count or error change is a partial refresh of that region",
     a_count_or_error_change_is_a_partial_refresh_of_that_region},
    {"a page or payload or link change is a full refresh", a_page_or_payload_or_link_change_is_a_full_refresh},
    {"the forced full refresh arrives at the bound and any full refresh resets it",
     the_forced_full_refresh_arrives_at_the_bound_and_any_full_refresh_resets_it},
    {"a forced full refresh is always full and resets the count",
     a_forced_full_refresh_is_always_full_and_resets_the_count},
};

int main(void) {
    return remote_test_run_all(test_cases, REMOTE_TEST_ROW_COUNT(test_cases));
}
