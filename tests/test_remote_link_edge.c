/*
 * KE4 tests first (Pico spec, KE4): the transport glue's decision table,
 * total over every observed fact and prior state, and the same as the
 * Flipper's, against which the appliance's behaviour was proven: the port
 * is open exactly when the cable is present and the host has asserted DTR,
 * the session hears only the edges, and bytes are delivered only while open.
 */
#include "test_support.h"

#include "../transport/remote_link_edge.h"

typedef struct {
    bool was_open;
    bool cable_present;
    bool host_opened;
    RemoteLinkEdgeOutcome expected;
    const char* description;
} EdgeRow;

/* Every combination: two prior states by four observations. */
static const EdgeRow edge_rows[] = {
    {false, false, false, RemoteLinkEdgeNone, "closed, nothing: stays closed"},
    {false, false, true, RemoteLinkEdgeNone, "closed, DTR without a cable (stale): stays closed"},
    {false, true, false, RemoteLinkEdgeNone, "closed, cable but host has not opened: stays closed"},
    {false, true, true, RemoteLinkEdgeOpened, "closed, cable and DTR: opens"},
    {true, false, false, RemoteLinkEdgeClosed, "open, cable pulled and DTR gone: closes"},
    {true, false, true, RemoteLinkEdgeClosed, "open, cable pulled with DTR cached: closes"},
    {true, true, false, RemoteLinkEdgeClosed, "open, host closed the port: closes"},
    {true, true, true, RemoteLinkEdgeNone, "open, still both: stays open"},
};

static void the_decision_table_is_total_and_opens_only_on_cable_and_dtr_together(RemoteTestReport* report) {
    for(int row_index = 0; row_index < REMOTE_TEST_ROW_COUNT(edge_rows); row_index++) {
        const EdgeRow* row = &edge_rows[row_index];
        RemoteLinkEdge edge;
        remote_link_edge_initialise(&edge);
        edge.port_open = row->was_open;
        RemoteLinkEdgeOutcome outcome = remote_link_edge_observe(&edge, row->cable_present, row->host_opened);
        REMOTE_TEST_ASSERT_EQUAL_INT(report, row->expected, outcome, row->description);
        REMOTE_TEST_ASSERT_EQUAL_INT(
            report, row->cable_present && row->host_opened, remote_link_edge_is_open(&edge), row->description);
    }
}

static void an_edge_is_reported_once_and_repeats_are_silent(RemoteTestReport* report) {
    RemoteLinkEdge edge;
    remote_link_edge_initialise(&edge);
    REMOTE_TEST_ASSERT(report, !remote_link_edge_is_open(&edge), "starts closed");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeOpened, remote_link_edge_observe(&edge, true, true), "opens");
    for(int repeat = 0; repeat < 5; repeat++) {
        REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeNone, remote_link_edge_observe(&edge, true, true), "silent");
    }
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeClosed, remote_link_edge_observe(&edge, true, false), "closes");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeNone, remote_link_edge_observe(&edge, false, false), "silent");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeOpened, remote_link_edge_observe(&edge, true, true), "opens again");
}

/* A cable pull and reinsertion with the host reopening is two edges, so
 * the session handshakes afresh rather than believing the old link. */
static void a_cable_pull_and_reinsertion_is_a_close_then_an_open(RemoteTestReport* report) {
    RemoteLinkEdge edge;
    remote_link_edge_initialise(&edge);
    remote_link_edge_observe(&edge, true, true);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeClosed, remote_link_edge_observe(&edge, false, false), "pull");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeNone, remote_link_edge_observe(&edge, true, false), "reinserted, host not yet open");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteLinkEdgeOpened, remote_link_edge_observe(&edge, true, true), "host reopened");
}

static const RemoteTestCase test_cases[] = {
    {"the decision table is total and opens only on cable and dtr together",
     the_decision_table_is_total_and_opens_only_on_cable_and_dtr_together},
    {"an edge is reported once and repeats are silent", an_edge_is_reported_once_and_repeats_are_silent},
    {"a cable pull and reinsertion is a close then an open", a_cable_pull_and_reinsertion_is_a_close_then_an_open},
};

int main(void) {
    return remote_test_run_all(test_cases, REMOTE_TEST_ROW_COUNT(test_cases));
}
