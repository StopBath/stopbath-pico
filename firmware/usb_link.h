/*
 * The USB CDC transport for the peripheral link (Pico spec KE4): the
 * appliance appears to the Pico as the host that opens a serial port.
 *
 * This is the SDK facing edge of the link, so it is deliberately thin: it
 * moves bytes between TinyUSB's CDC interface and a RemoteSession, and
 * turns the two facts TinyUSB reports (the bus mounted and not suspended;
 * the host asserting DTR) into the session's port opened and port closed
 * events through the decision table in transport/remote_link_edge.c, which
 * is tested on the host. Every decision about the bytes lives in the
 * session, which is tested on the host; nothing here is.
 *
 * Evidence, all in the pinned SDK's lib/tinyusb (0.18.0), evaluation log
 * 4.2: tud_cdc_n_connected is tud_ready and the DTR bit; cdcd_reset clears
 * the DTR bit on a bus reset, so a re-enumeration cannot carry a stale
 * value; tud_task must be called from the loop to service the stack.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "../session/remote_session.h"

/* Starts the USB stack. The session is the caller's and outlives the link.
 * Returns false if the stack refused to start, in which case the link never
 * opens and the caller says so on the panel. */
bool usb_link_initialise(RemoteSession* session);

/* Called every tick: services the stack, signals the session on a link
 * edge, feeds received bytes in and sends queued bytes out, and advances
 * the session's handshake clock. Never blocks. */
void usb_link_service(uint32_t elapsed_milliseconds);

/* Whether the host currently has the port open, for the diagnostic line. */
bool usb_link_port_is_open(void);
