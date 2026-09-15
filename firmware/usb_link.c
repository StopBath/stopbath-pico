#include "usb_link.h"

#include "tusb.h"

#include "../transport/remote_link_edge.h"

/* The one CDC interface (tusb_config.h, CFG_TUD_CDC 1). */
#define LINK_INTERFACE 0

/* Read in chunks of the CDC FIFO's size so one service drains everything
 * that arrived; sized once at start (Flipper 0.10). */
#define READ_CHUNK 512

static RemoteSession* link_session;
static RemoteLinkEdge link_edge;
static uint8_t read_buffer[READ_CHUNK];
static uint8_t write_buffer[REMOTE_PROTOCOL_MAXIMUM_MESSAGE_LENGTH];

bool usb_link_initialise(RemoteSession* session) {
    link_session = session;
    remote_link_edge_initialise(&link_edge);
    return tusb_init();
}

static void service_link_edge(void) {
    /* The two facts, separately, so the table reads as the Flipper's; on
     * this stack tud_cdc_n_connected already combines them. */
    bool cable_present = tud_ready();
    bool host_opened = tud_cdc_n_connected(LINK_INTERFACE);
    switch(remote_link_edge_observe(&link_edge, cable_present, host_opened)) {
    case RemoteLinkEdgeOpened:
        remote_session_port_opened(link_session);
        break;
    case RemoteLinkEdgeClosed:
        remote_session_port_closed(link_session);
        break;
    case RemoteLinkEdgeNone:
        break;
    }
}

static void service_receive(void) {
    while(tud_cdc_n_available(LINK_INTERFACE) > 0) {
        uint32_t received = tud_cdc_n_read(LINK_INTERFACE, read_buffer, sizeof(read_buffer));
        if(received == 0) {
            break;
        }
        /* Drained whether or not the port is open, so stale bytes do not
         * accumulate; delivered only while it is. */
        if(remote_link_edge_is_open(&link_edge)) {
            remote_session_receive(link_session, read_buffer, received);
        }
    }
}

static void service_send(void) {
    /* One message at a time, and only when the whole of it fits the CDC
     * FIFO, so a line is never split across a full buffer. What does not
     * fit stays queued in the session for the next service. */
    while(remote_link_edge_is_open(&link_edge)) {
        uint32_t room = tud_cdc_n_write_available(LINK_INTERFACE);
        if(room < sizeof(write_buffer)) {
            break;
        }
        size_t taken = remote_session_take_output(link_session, write_buffer, sizeof(write_buffer));
        if(taken == 0) {
            break;
        }
        tud_cdc_n_write(LINK_INTERFACE, write_buffer, (uint32_t)taken);
        tud_cdc_n_write_flush(LINK_INTERFACE);
    }
}

void usb_link_service(uint32_t elapsed_milliseconds) {
    tud_task();
    service_link_edge();
    service_receive();
    remote_session_tick(link_session, elapsed_milliseconds);
    service_send();
}

bool usb_link_port_is_open(void) {
    return remote_link_edge_is_open(&link_edge);
}
