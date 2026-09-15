/*
 * The remote: the program the device runs in the field (Pico spec 1.2).
 *
 * One loop, every KEYS_SAMPLE_PERIOD_MILLISECONDS: sample the keys and hand
 * classified presses to the session; service the USB link, which moves
 * bytes and link edges between TinyUSB and the session; ask the session what
 * the panel should show, add the two facts the session does not know (the
 * diagnostic screen toggle and the refresh counters), and let the display
 * refresher draw it without blocking.
 *
 * Nothing here decides what a press means. The session turns it into a
 * protocol event (with the one Key1 choice of spec 2.3) and the appliance
 * decides the rest (Flipper 2.1). The only local gesture is KEY1 long while
 * the link is down, which shows the diagnostic screen (spec 2.10 guidance):
 * Key1 has no reported meaning then, and a device that has stopped talking
 * should be able to say why without a computer.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "hardware/timer.h"
#include "pico/stdlib.h"

#include "../remote_display/remote_display_layout.h"
#include "../remote_display/remote_font.h"
#include "../remote_input/remote_input_model.h"
#include "../session/remote_session.h"
#include "DEV_Config.h"
#include "display_refresher.h"
#include "keys.h"
#include "panel.h"
#include "usb_link.h"

static RemoteSession session;
static DisplayRefresher refresher;
static RemoteInputModel input_model;
static bool show_diagnostics;
static bool usb_failed_to_start;

/* Drawn over every link screen if the USB stack refused to start: the one
 * fault the session cannot report because it never hears about it. */
static void draw_usb_failure(const RemoteDisplayState* state, void* context) {
    (void)context;
    if(!usb_failed_to_start || state->link_connected) {
        return;
    }
    const char* text = "USB STACK FAILED TO START";
    int width = remote_font_text_width(text, 2);
    remote_font_draw_text(&display_refresher_frame, (REMOTE_BITMAP_WIDTH - width) / 2, REMOTE_BITMAP_HEIGHT - 30, text, 2, RemoteBitmapBlack);
}

/* What the panel should show now: the session's view plus the local
 * diagnostic toggle and the refresher's own counters. */
static void compose_target(RemoteDisplayState* target) {
    remote_session_display(&session, target);
    target->show_diagnostics = show_diagnostics;
    target->diagnostics.full_refreshes = refresher.full_refreshes;
    target->diagnostics.partial_refreshes = refresher.partial_refreshes;
    target->diagnostics.last_refresh_milliseconds = refresher.last_refresh_milliseconds;
}

static void apply_press(RemoteInputOutcome outcome) {
    if(outcome.key == RemoteInputKey1 && outcome.press_kind == RemoteInputPressLong) {
        if(!usb_link_port_is_open() || show_diagnostics) {
            show_diagnostics = !show_diagnostics;
        }
        /* Connected and not showing diagnostics: reserved, nothing (spec 2.2). */
        return;
    }
    remote_session_report_press(&session, outcome.key, outcome.press_kind);
}

int main(void) {
    keys_initialise();
    DEV_Module_Init();
    panel_initialise();

    remote_session_initialise(&session);
    remote_input_model_initialise(&input_model);
    show_diagnostics = false;

    RemoteDisplayState target;
    compose_target(&target);
    display_refresher_initialise(&refresher, &target, draw_usb_failure, NULL);

    /* The link comes up after the first draw so the not connected screen is
     * on the glass before the host can open the port. */
    usb_failed_to_start = !usb_link_initialise(&session);
    if(usb_failed_to_start) {
        /* Force a redraw so the message appears; the state itself is the same. */
        remote_refresh_policy_force_full(&refresher.policy);
        panel_show_full_frame(display_refresher_frame.bytes);
    }

    uint64_t last_sample_microseconds = time_us_64();
    while(true) {
        sleep_ms(KEYS_SAMPLE_PERIOD_MILLISECONDS);
        uint64_t now_microseconds = time_us_64();
        uint32_t elapsed_milliseconds = (uint32_t)((now_microseconds - last_sample_microseconds) / 1000u);
        last_sample_microseconds = now_microseconds;

        for(int key = 0; key < (int)RemoteInputKeyCount; key++) {
            RemoteInputOutcome outcome =
                remote_input_model_observe(&input_model, key, keys_is_pressed((RemoteInputKey)key), elapsed_milliseconds);
            if(outcome.kind == RemoteInputOutcomeClassifiedPress) {
                apply_press(outcome);
            }
        }

        usb_link_service(elapsed_milliseconds);

        /* The diagnostic screen is a snapshot: its counters are refreshed
         * only when it is not already showing, so a refresh's own duration
         * cannot make the screen differ from itself (the KE2 lesson). */
        compose_target(&target);
        if(show_diagnostics && refresher.target.show_diagnostics) {
            target.diagnostics = refresher.target.diagnostics;
        }
        display_refresher_set_target(&refresher, &target);
        display_refresher_service(&refresher);
    }
}
