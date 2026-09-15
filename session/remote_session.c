#include "remote_session.h"

#include <string.h>

static void reset_display_record(RemoteProtocolMessage* record) {
    remote_protocol_message_initialise(record, RemoteProtocolVerbDisplay);
    remote_protocol_message_set_integer(record, RemoteProtocolDisplayFieldStatus, RemoteProtocolStatusReady);
    remote_protocol_message_set_integer(record, RemoteProtocolDisplayFieldPage, RemoteProtocolPageNone);
    remote_protocol_message_set_text(record, RemoteProtocolDisplayFieldPayload, "");
    remote_protocol_message_set_integer(record, RemoteProtocolDisplayFieldDelivered, 0);
    remote_protocol_message_set_integer(record, RemoteProtocolDisplayFieldError, RemoteProtocolErrorNone);
}

void remote_session_initialise(RemoteSession* session) {
    memset(session, 0, sizeof(*session));
    session->link_state = RemoteSessionLinkDown;
    reset_display_record(&session->current_display);
    remote_protocol_line_assembler_initialise(&session->inbound);
}

static void clear_output(RemoteSession* session) {
    session->output_length = 0;
    session->output_message_count = 0;
}

/* Appends an encoded message to the output, or drops it and counts why. A
 * message that does not fit is dropped whole; nothing is ever written past
 * the buffer. Returns true if it was queued. */
static bool queue_message(RemoteSession* session, const RemoteProtocolMessage* message, uint32_t* drop_counter) {
    uint8_t line[REMOTE_PROTOCOL_MAXIMUM_MESSAGE_LENGTH];
    size_t line_length = 0;
    if(!remote_protocol_encode(message, (char*)line, sizeof(line), &line_length)) {
        return false;
    }
    if(session->output_message_count >= REMOTE_SESSION_OUTBOUND_QUEUE_DEPTH ||
       session->output_length + line_length > sizeof(session->output)) {
        if(drop_counter != NULL) {
            (*drop_counter)++;
        }
        return false;
    }
    memcpy(session->output + session->output_length, line, line_length);
    session->output_length += line_length;
    session->output_message_count++;
    return true;
}

static void send_hello(RemoteSession* session) {
    RemoteProtocolMessage hello;
    remote_protocol_message_initialise(&hello, RemoteProtocolVerbHello);
    remote_protocol_message_set_integer(&hello, RemoteProtocolHelloFieldVersion, REMOTE_PROTOCOL_VERSION);
    remote_protocol_message_set_text(&hello, RemoteProtocolHelloFieldPeripheral, REMOTE_SESSION_PERIPHERAL_TOKEN);
    /* No lock on this device (KD5); reported honestly. */
    remote_protocol_message_set_integer(&hello, RemoteProtocolHelloFieldLocked, 0);
    queue_message(session, &hello, NULL);
}

void remote_session_port_opened(RemoteSession* session) {
    /* A fresh handshake every time, discarding any prior link state and any
     * half received line (Flipper 2.10). */
    remote_protocol_line_assembler_initialise(&session->inbound);
    clear_output(session);
    reset_display_record(&session->current_display);
    session->link_state = RemoteSessionHandshaking;
    session->handshake_elapsed_milliseconds = 0;
    send_hello(session);
}

void remote_session_tick(RemoteSession* session, uint32_t elapsed_milliseconds) {
    if(session->link_state != RemoteSessionHandshaking) {
        session->handshake_elapsed_milliseconds = 0;
        return;
    }
    session->handshake_elapsed_milliseconds += elapsed_milliseconds;
    if(session->handshake_elapsed_milliseconds < REMOTE_SESSION_HANDSHAKE_RETRY_INTERVAL_MILLISECONDS) {
        return;
    }
    session->handshake_elapsed_milliseconds = 0;
    session->handshake_retries++;
    /* Only a stale HELLO can be queued while handshaking, so clearing the
     * output leaves exactly one fresh HELLO rather than copies piling up. */
    clear_output(session);
    send_hello(session);
}

void remote_session_port_closed(RemoteSession* session) {
    if(session->link_state != RemoteSessionLinkDown) {
        session->reconnections++;
    }
    /* Discard link state and clear the sensitive payload. Anything not yet
     * drained is dropped, which is what stops a stale press crossing the
     * reconnection. */
    remote_protocol_line_assembler_initialise(&session->inbound);
    clear_output(session);
    reset_display_record(&session->current_display);
    session->link_state = RemoteSessionLinkDown;
}

static void handle_display(RemoteSession* session, const RemoteProtocolMessage* record) {
    uint32_t error = record->fields[RemoteProtocolDisplayFieldError].integer;
    if(session->link_state != RemoteSessionConnected && error == RemoteProtocolErrorBadVersion) {
        /* The appliance will not talk to this version (Flipper 2.10 step 2). */
        session->version_mismatches++;
        session->link_state = RemoteSessionIncompatible;
        return;
    }
    session->current_display = *record;
    session->link_state = RemoteSessionConnected;
}

void remote_session_receive(RemoteSession* session, const uint8_t* bytes, size_t byte_count) {
    RemoteProtocolMessage message;
    for(size_t index = 0; index < byte_count; index++) {
        RemoteProtocolFeedOutcome outcome = remote_protocol_feed_byte(&session->inbound, bytes[index], &message);
        if(outcome.kind == RemoteProtocolFeedOutcomeMessage) {
            if(message.verb == RemoteProtocolVerbDisplay) {
                handle_display(session, &message);
            } else {
                /* Only DISPLAY flows from the appliance; anything else is a
                 * peer speaking out of turn and is treated as malformed. */
                session->malformed_received++;
            }
        } else if(outcome.kind == RemoteProtocolFeedOutcomeError) {
            session->malformed_received++;
        }
    }
}

RemoteProtocolEvent remote_session_page_event_for_key1(int page) {
    /* Spec 2.3, KD4: the appliance's page events are absolute (LEFT_SHORT is
     * the WIFI page, RIGHT_SHORT the GUEST page), and this device has one
     * key to move between them. From the GUEST page the other page is WIFI;
     * from anywhere else, including no page and a value the enumeration
     * does not name, it is GUEST, which the appliance answers with an
     * unchanged record outside a session. Recorded as a deviation from
     * Flipper 2.1 in IMPLEMENTATION_DEVIATIONS.md; provisional until KD9. */
    return page == (int)RemoteProtocolPageGuest ? RemoteProtocolEventLeftShort : RemoteProtocolEventRightShort;
}

bool remote_session_report_press(RemoteSession* session, RemoteInputKey key, RemoteInputPressKind press_kind) {
    RemoteProtocolEvent wire_event;
    if(key == RemoteInputKey0 && press_kind == RemoteInputPressShort) {
        wire_event = RemoteProtocolEventCenterShort;
    } else if(key == RemoteInputKey0 && press_kind == RemoteInputPressLong) {
        wire_event = RemoteProtocolEventCenterLong;
    } else if(key == RemoteInputKey1 && press_kind == RemoteInputPressShort) {
        wire_event = remote_session_page_event_for_key1(
            (int)session->current_display.fields[RemoteProtocolDisplayFieldPage].integer);
    } else {
        /* Key1 long is reserved (spec 2.2); anything else is not a press
         * this table names. Neither is a fault, so neither is counted. */
        return false;
    }
    if(session->link_state != RemoteSessionConnected) {
        session->events_dropped_no_link++;
        return false;
    }

    RemoteProtocolMessage button;
    remote_protocol_message_initialise(&button, RemoteProtocolVerbButton);
    remote_protocol_message_set_integer(&button, RemoteProtocolButtonFieldEvent, wire_event);
    /* Both true, honestly (spec 2.4): single purpose firmware, no lock. */
    remote_protocol_message_set_integer(&button, RemoteProtocolButtonFieldForegrounded, 1);
    remote_protocol_message_set_integer(&button, RemoteProtocolButtonFieldUnlocked, 1);
    return queue_message(session, &button, &session->events_dropped_by_output_full);
}

size_t remote_session_take_output(RemoteSession* session, uint8_t* destination, size_t destination_capacity) {
    size_t taken = session->output_length < destination_capacity ? session->output_length : destination_capacity;
    memcpy(destination, session->output, taken);
    memmove(session->output, session->output + taken, session->output_length - taken);
    session->output_length -= taken;
    /* Recount whole messages remaining by their terminators, so a partial
     * drain leaves the count honest. */
    session->output_message_count = 0;
    for(size_t index = 0; index < session->output_length; index++) {
        if(session->output[index] == REMOTE_PROTOCOL_TERMINATOR) {
            session->output_message_count++;
        }
    }
    return taken;
}

static void copy_bounded(char* destination, size_t capacity, const char* source) {
    size_t length = 0;
    while(length + 1 < capacity && source[length] != '\0') {
        destination[length] = source[length];
        length++;
    }
    destination[length] = '\0';
}

void remote_session_display(const RemoteSession* session, RemoteDisplayState* display_state) {
    remote_display_state_initialise(display_state);
    display_state->diagnostics.reconnections = session->reconnections;
    display_state->diagnostics.malformed_received = session->malformed_received;
    display_state->diagnostics.version_mismatches = session->version_mismatches;
    display_state->diagnostics.events_dropped_no_link = session->events_dropped_no_link;
    display_state->diagnostics.events_dropped_by_output_full = session->events_dropped_by_output_full;
    display_state->diagnostics.handshake_retries = session->handshake_retries;

    switch(session->link_state) {
    case RemoteSessionLinkDown:
        display_state->link_connected = false;
        break;
    case RemoteSessionHandshaking:
        display_state->link_connected = false;
        display_state->link_connecting = true;
        break;
    case RemoteSessionIncompatible:
        display_state->link_connected = false;
        display_state->link_incompatible = true;
        break;
    case RemoteSessionConnected: {
        display_state->link_connected = true;
        const RemoteProtocolMessage* record = &session->current_display;
        display_state->status = (int)record->fields[RemoteProtocolDisplayFieldStatus].integer;
        display_state->page = (int)record->fields[RemoteProtocolDisplayFieldPage].integer;
        copy_bounded(display_state->payload, sizeof(display_state->payload), record->fields[RemoteProtocolDisplayFieldPayload].text);
        display_state->delivered_count = record->fields[RemoteProtocolDisplayFieldDelivered].integer;
        /* The error field becomes the display's error code text through the
         * enumeration's wire names, so the code shown is the code sent. NONE
         * leaves the band off. */
        uint32_t error = record->fields[RemoteProtocolDisplayFieldError].integer;
        if(error != RemoteProtocolErrorNone) {
            int error_count = 0;
            const char* const* error_names = remote_protocol_enumeration_values(RemoteProtocolEnumerationError, &error_count);
            if((int)error < error_count) {
                copy_bounded(display_state->error_code, sizeof(display_state->error_code), error_names[error]);
            }
        }
        break;
    }
    }
}
