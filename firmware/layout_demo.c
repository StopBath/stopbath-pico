/*
 * KE2 layout demo: every fixture on the real panel, the refresh policy in
 * action, and a stopwatch on each refresh (Pico spec, KE2 hardware gate;
 * cleared by the author 2026-09-15, kept as the way to judge a layout
 * change on the glass).
 *
 * Controls, for the author judging the panel:
 *   KEY1 short   next fixture, in the order of remote_display_fixtures.c
 *                (the three link screens first, diagnostics last, then
 *                round again); a full refresh each, since the page or link
 *                screen changes. The fixture number is drawn in the spare
 *                strip of the column so the sequence is followable
 *   KEY0 short   one more photograph delivered (a partial refresh of the
 *                count region; the code must not move)
 *   KEY0 long    toggle the error band (a partial refresh of the band)
 *   KEY1 long    toggle the diagnostic screen, showing the counters as they
 *                stood at the toggle: full and partial refreshes so far and
 *                the duration of the refresh before the toggle, in
 *                milliseconds. A snapshot, deliberately: feeding each
 *                finished refresh's duration back into the screen made the
 *                screen differ from itself and refresh without end (the
 *                author's observation, 2026-09-15)
 *
 * The loop never waits on the panel; display_refresher.c is what makes a
 * press during a refresh land the moment the panel is free.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "hardware/timer.h"
#include "pico/stdlib.h"

#include "../remote_display/remote_display_fixtures.h"
#include "../remote_display/remote_font.h"
#include "../remote_input/remote_input_model.h"
#include "DEV_Config.h"
#include "display_refresher.h"
#include "keys.h"
#include "panel.h"

typedef struct {
    DisplayRefresher refresher;
    RemoteDisplayState target;
    int fixture_index;
} DemoState;

static RemoteDisplayDiagnostics live_counters(const DemoState* demo) {
    RemoteDisplayDiagnostics live;
    memset(&live, 0, sizeof(live));
    live.full_refreshes = demo->refresher.full_refreshes;
    live.partial_refreshes = demo->refresher.partial_refreshes;
    live.last_refresh_milliseconds = demo->refresher.last_refresh_milliseconds;
    return live;
}

static void load_fixture(DemoState* demo, int fixture_index) {
    demo->fixture_index = fixture_index;
    demo->target = remote_display_fixtures()[fixture_index].display_state;
    if(demo->target.show_diagnostics) {
        demo->target.diagnostics = live_counters(demo);
    }
}

static void apply_press(DemoState* demo, RemoteInputOutcome outcome) {
    if(outcome.key == RemoteInputKey1 && outcome.press_kind == RemoteInputPressShort) {
        load_fixture(demo, (demo->fixture_index + 1) % remote_display_fixture_count());
    } else if(outcome.key == RemoteInputKey0 && outcome.press_kind == RemoteInputPressShort) {
        demo->target.delivered_count++;
    } else if(outcome.key == RemoteInputKey0 && outcome.press_kind == RemoteInputPressLong) {
        if(demo->target.error_code[0] == '\0') {
            strncpy(demo->target.error_code, REMOTE_DISPLAY_FIXTURE_ERROR_CODE, REMOTE_DISPLAY_CODE_CAPACITY - 1);
        } else {
            demo->target.error_code[0] = '\0';
        }
    } else if(outcome.key == RemoteInputKey1 && outcome.press_kind == RemoteInputPressLong) {
        demo->target.show_diagnostics = !demo->target.show_diagnostics;
        demo->target.diagnostics = live_counters(demo);
    }
    display_refresher_set_target(&demo->refresher, &demo->target);
}

/* Demo only: the fixture number, drawn after the layout in the strip of the
 * column no region claims (between the count and the error band), so it
 * rides along with full refreshes and is never part of a partial one. Eleven
 * cells fit the column at this size; "FIXTURE 18/18" is thirteen and lost
 * its last digit on the glass (author, 2026-09-15). */
static void draw_fixture_number(const RemoteDisplayState* state, void* context) {
    const DemoState* demo = context;
    if(state->show_diagnostics || !state->link_connected) {
        return;
    }
    char label[24] = "FXTR ";
    size_t used = 5;
    int number = demo->fixture_index + 1;
    if(number >= 10) {
        label[used++] = (char)('0' + number / 10);
    }
    label[used++] = (char)('0' + number % 10);
    label[used++] = '/';
    int total = remote_display_fixture_count();
    if(total >= 10) {
        label[used++] = (char)('0' + total / 10);
    }
    label[used++] = (char)('0' + total % 10);
    label[used] = '\0';
    RemoteLayoutRectangle count_area = remote_display_layout_region(RemoteLayoutRegionDelivered);
    RemoteLayoutRectangle error_area = remote_display_layout_region(RemoteLayoutRegionError);
    int strip_top = count_area.y + count_area.height;
    int strip_height = error_area.y - strip_top;
    remote_font_draw_text(&display_refresher_frame, count_area.x + 6, strip_top + (strip_height - 14) / 2, label, 2, RemoteBitmapBlack);
}

int main(void) {
    keys_initialise();
    DEV_Module_Init();
    panel_initialise();

    DemoState demo;
    memset(&demo, 0, sizeof(demo));
    load_fixture(&demo, 3);
    display_refresher_initialise(&demo.refresher, &demo.target, draw_fixture_number, &demo);

    RemoteInputModel input_model;
    remote_input_model_initialise(&input_model);

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
                apply_press(&demo, outcome);
            }
        }
        display_refresher_service(&demo.refresher);
    }
}
