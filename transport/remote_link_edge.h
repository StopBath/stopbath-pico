/*
 * The transport's decision table, kept out of the SDK edge so it can be
 * proven on the host (Pico spec KE4): the port is open exactly when the
 * cable is present and the host has opened the port (DTR asserted), and the
 * session hears only the edges.
 *
 * This is the Flipper transport's rule (its remote_transport.c: port open is
 * usb_present and dtr_present, signalled on change), against which the
 * appliance's link handling was proven on hardware. On the Pico the two
 * facts arrive combined: TinyUSB's tud_cdc_n_connected is tud_ready (mounted
 * and not suspended) and the DTR bit, and a bus reset clears the DTR bit
 * (cdcd_reset, evaluation log 4.2), so no stale value survives a
 * re-enumeration. The firmware still passes the two facts separately so the
 * table here reads as the Flipper's does.
 */
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RemoteLinkEdgeNone,
    RemoteLinkEdgeOpened,
    RemoteLinkEdgeClosed,
} RemoteLinkEdgeOutcome;

typedef struct {
    bool port_open;
} RemoteLinkEdge;

/* Starts closed: a port already open when the firmware starts is noticed on
 * the first observation as an open edge, which is what starts the handshake. */
void remote_link_edge_initialise(RemoteLinkEdge* edge);

/* One observation of the two facts. Returns an edge when the open state
 * changed, otherwise none. */
RemoteLinkEdgeOutcome remote_link_edge_observe(RemoteLinkEdge* edge, bool cable_present, bool host_opened);

bool remote_link_edge_is_open(const RemoteLinkEdge* edge);

#ifdef __cplusplus
}
#endif
