/*
 * The client session: the state machine between the transport, the input
 * model and the display. Pure logic, no SDK, so the whole of it is tested on
 * the development machine (Pico spec KE3).
 *
 * Modelled on the Flipper repository's session/remote_session.c at commit
 * 3677e61904d854504ac58c5dce6d6e6419836cfc, which proved this shape against
 * the real appliance; written fresh here because that one composes the
 * Flipper's display and carries a screen lock this device has not got.
 *
 * What it does, and the rules it enforces:
 *
 * - Handshake (Flipper 2.10): when the host opens the port it sends HELLO
 *   with the protocol version, the token stopbath-pico (KD7) and locked=0
 *   (KD5), and waits. The first DISPLAY the appliance answers with is the
 *   acceptance; a DISPLAY carrying BAD_VERSION is a rejection and the link
 *   is shown incompatible. A lost handshake is retried on an interval.
 * - State replacement (Flipper 2.5): it holds no authoritative state. Each
 *   DISPLAY replaces the last one wholly. On a link drop it discards
 *   everything and shows not connected, never stale content.
 * - Presses (spec 2.2, 2.3): a press is sent only while connected, never
 *   queued for later, so a stale press cannot arrive after a reconnection
 *   and end a later session (Flipper 2.10, prohibition 8). Key0 short and
 *   long are CENTER_SHORT and CENTER_LONG. Key1 short is the one place this
 *   device chooses: RIGHT_SHORT unless the last record named the GUEST page,
 *   then LEFT_SHORT, in remote_session_page_event_for_key1 and nowhere else.
 *   Key1 long is reserved and sends nothing.
 * - The guard flags: foregrounded=1 and unlocked=1 on every BUTTON, honestly
 *   (spec 2.4): single purpose firmware, no lock. The appliance enforces
 *   them regardless.
 * - Sensitive state (Flipper Part 5): the payload, which may carry the
 *   Wi-Fi passphrase, is cleared the moment the link drops.
 *
 * It interprets nothing about what a button means beyond that one choice;
 * meaning is the appliance's (Flipper 2.1).
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../protocol/remote_protocol.h"
#include "../remote_display/remote_display_layout.h"
#include "../remote_input/remote_input_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The identifier this peripheral sends in HELLO (KD7), shown on the
 * appliance's dashboard. A protocol token: lower case, digits, hyphens. */
#define REMOTE_SESSION_PERIPHERAL_TOKEN "stopbath-pico"

/* How many outbound messages the session holds before dropping the next and
 * counting it. This peripheral's own constant, not a protocol bound: it
 * bounds this device's input path and the appliance never sees it. A press
 * that would exceed it is dropped, never queued across a disconnection. */
#define REMOTE_SESSION_OUTBOUND_QUEUE_DEPTH 4

/* How long to wait for the appliance's DISPLAY acceptance before re-sending
 * HELLO. The appliance treats a second HELLO as a restart and resends the
 * record (seam 4.2), so a periodic retry recovers a handshake that would
 * otherwise hang. Well under the inbound rate bound. */
#define REMOTE_SESSION_HANDSHAKE_RETRY_INTERVAL_MILLISECONDS 2000

typedef enum {
    /* The host has not opened the port: no cable, or the appliance has not
     * opened it. Nothing is sent; the display shows not connected. */
    RemoteSessionLinkDown,
    /* The port is open and HELLO has been sent; awaiting the first DISPLAY. */
    RemoteSessionHandshaking,
    /* A DISPLAY has been received; rendering it. */
    RemoteSessionConnected,
    /* The appliance answered HELLO with BAD_VERSION. */
    RemoteSessionIncompatible,
} RemoteSessionLinkState;

typedef struct {
    RemoteSessionLinkState link_state;
    /* The inbound line assembler, reset whenever the link drops so no partial
     * line survives a reconnection. */
    RemoteProtocolLineAssembler inbound;
    /* The last DISPLAY received, held only while connected. */
    RemoteProtocolMessage current_display;
    /* Bytes waiting to be sent by the transport. Bounded; a press that would
     * overflow it is dropped and counted, never queued unboundedly. */
    uint8_t output[REMOTE_PROTOCOL_MAXIMUM_MESSAGE_LENGTH * REMOTE_SESSION_OUTBOUND_QUEUE_DEPTH];
    size_t output_length;
    int output_message_count;
    /* Milliseconds spent in the handshake since the last HELLO. */
    uint32_t handshake_elapsed_milliseconds;
    /* Diagnostics for the screen Flipper 2.11 asks for. */
    uint32_t reconnections;
    uint32_t malformed_received;
    uint32_t version_mismatches;
    uint32_t events_dropped_no_link;
    uint32_t events_dropped_by_output_full;
    uint32_t handshake_retries;
} RemoteSession;

void remote_session_initialise(RemoteSession* session);

/* The host opened the port (DTR asserted) or the peripheral restarted: send
 * HELLO and begin the handshake. Discards any prior link state first. */
void remote_session_port_opened(RemoteSession* session);

/* The host closed the port, the cable was pulled, or USB suspended: discard
 * link state, clear the sensitive payload, and drop anything unsent. */
void remote_session_port_closed(RemoteSession* session);

/* Advances the handshake retry clock. While handshaking, re-sends HELLO once
 * the interval passes. A no-op in every other link state. */
void remote_session_tick(RemoteSession* session, uint32_t elapsed_milliseconds);

/* Feeds bytes received from the appliance. Complete DISPLAY records replace
 * the current one; a BAD_VERSION answer marks the link incompatible;
 * malformed input is counted. Any other verb from the appliance is counted
 * as malformed, since only DISPLAY flows this way. */
void remote_session_receive(RemoteSession* session, const uint8_t* bytes, size_t byte_count);

/* A classified press from the input model. Transmits BUTTON only while
 * connected; otherwise drops it and counts why. A Key1 long press is
 * reserved and sends nothing. Returns true if a message was queued. */
bool remote_session_report_press(RemoteSession* session, RemoteInputKey key, RemoteInputPressKind press_kind);

/* The Key1 choice of spec 2.3, in one place: LEFT_SHORT when the page is
 * GUEST, RIGHT_SHORT for every other value including ones outside the
 * enumeration. Takes a plain integer so a test can pass any value. Can
 * return nothing but those two events. */
RemoteProtocolEvent remote_session_page_event_for_key1(int page);

/* Drains bytes the transport should send, clearing them. Returns the count. */
size_t remote_session_take_output(RemoteSession* session, uint8_t* destination, size_t destination_capacity);

/* Fills the display state to render: the appliance's record when connected,
 * a link screen otherwise, with the session's counters in the diagnostics.
 * show_diagnostics is left false for the firmware to set. */
void remote_session_display(const RemoteSession* session, RemoteDisplayState* display_state);

#ifdef __cplusplus
}
#endif
