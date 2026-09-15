/*
 * KE2 layout demo: every fixture on the real panel, the refresh policy in
 * action, and a stopwatch on each refresh (Pico spec, KE2 hardware gate).
 *
 * Controls, for the author judging the panel:
 *   KEY1 short   next fixture (a full refresh, since the page or link
 *                screen changes)
 *   KEY0 short   one more photograph delivered (a partial refresh of the
 *                count region; the code must not move)
 *   KEY0 long    toggle the error band (a partial refresh of the band)
 *   KEY1 long    toggle the diagnostic screen, whose counters are live:
 *                full and partial refreshes so far and the last refresh's
 *                duration in milliseconds
 *
 * The loop never waits on the panel. A refresh is begun and the keys keep
 * being sampled; a press during a refresh changes the target state, and the
 * refresh it needs is begun the moment the panel is free. That is the
 * behaviour first light lacked.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"

#include "../remote_display/remote_bitmap.h"
#include "../remote_display/remote_display_fixtures.h"
#include "../remote_display/remote_display_layout.h"
#include "../remote_display/remote_refresh_policy.h"
#include "../remote_input/remote_input_model.h"
#include "DEV_Config.h"
#include "panel.h"

#define KEY0_GPIO 15u
#define KEY1_GPIO 17u
#define SAMPLE_PERIOD_MILLISECONDS 5u

static RemoteBitmap frame;

typedef struct {
    /* What the panel was last told to show, field by field. */
    RemoteDisplayState shown;
    /* What it should show next. */
    RemoteDisplayState target;
    RemoteRefreshPolicy policy;
    /* Regions still to send for the partial refresh in progress. */
    unsigned pending_regions;
    bool refresh_in_progress;
    RemoteRefreshKind refresh_kind;
    uint64_t refresh_started_microseconds;
    RemoteDisplayDiagnostics live;
    int fixture_index;
} DemoState;

static void configure_key(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, false);
    gpio_pull_up(gpio);
}

static bool key_is_pressed(uint gpio) {
    return !gpio_get(gpio);
}

static void load_fixture(DemoState* demo, int fixture_index) {
    demo->fixture_index = fixture_index;
    demo->target = remote_display_fixtures()[fixture_index].display_state;
    if(demo->target.show_diagnostics) {
        demo->target.diagnostics = demo->live;
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
        demo->target.diagnostics = demo->live;
    }
}

static void begin_next_partial_region(DemoState* demo) {
    for(int region = 0; region < RemoteLayoutRegionCount; region++) {
        unsigned bit = 1u << region;
        if(demo->pending_regions & bit) {
            demo->pending_regions &= ~bit;
            RemoteLayoutRectangle area = remote_display_layout_region((RemoteLayoutRegion)region);
            panel_begin_partial_refresh(frame.bytes, area.x, area.y, area.width, area.height);
            demo->refresh_in_progress = true;
            demo->refresh_kind = RemoteRefreshKindPartial;
            demo->refresh_started_microseconds = time_us_64();
            demo->live.partial_refreshes++;
            return;
        }
    }
}

/* Called only while the panel is free. Decides what the change from shown
 * to target costs, renders the target, and begins the first refresh. */
static void begin_refresh_if_needed(DemoState* demo) {
    if(demo->pending_regions != 0u) {
        begin_next_partial_region(demo);
        return;
    }
    RemoteRefreshDecision decision = remote_refresh_policy_decide(&demo->policy, &demo->shown, &demo->target);
    if(decision.kind == RemoteRefreshKindNone) {
        return;
    }
    remote_display_layout_render(&demo->target, &frame);
    demo->shown = demo->target;
    if(decision.kind == RemoteRefreshKindFull) {
        panel_begin_full_refresh(frame.bytes);
        demo->refresh_in_progress = true;
        demo->refresh_kind = RemoteRefreshKindFull;
        demo->refresh_started_microseconds = time_us_64();
        demo->live.full_refreshes++;
        return;
    }
    demo->pending_regions = decision.regions;
    begin_next_partial_region(demo);
}

static void note_refresh_finished(DemoState* demo) {
    demo->refresh_in_progress = false;
    uint64_t elapsed = time_us_64() - demo->refresh_started_microseconds;
    demo->live.last_refresh_milliseconds = (uint32_t)(elapsed / 1000u);
    /* If the diagnostic screen is up, it shows the figure just measured. */
    if(demo->target.show_diagnostics) {
        demo->target.diagnostics = demo->live;
    }
}

int main(void) {
    configure_key(KEY0_GPIO);
    configure_key(KEY1_GPIO);
    DEV_Module_Init();
    panel_initialise();

    DemoState demo;
    memset(&demo, 0, sizeof(demo));
    remote_refresh_policy_initialise(&demo.policy);
    remote_display_state_initialise(&demo.shown);
    /* The panel keeps whatever it last showed; the first draw is forced so
     * the shown record and the glass agree from the start. */
    load_fixture(&demo, 3);
    remote_display_layout_render(&demo.target, &frame);
    demo.shown = demo.target;
    remote_refresh_policy_force_full(&demo.policy);
    demo.refresh_started_microseconds = time_us_64();
    panel_show_full_frame(frame.bytes);
    demo.live.full_refreshes = 1;
    demo.live.last_refresh_milliseconds = (uint32_t)((time_us_64() - demo.refresh_started_microseconds) / 1000u);

    RemoteInputModel input_model;
    remote_input_model_initialise(&input_model);

    uint64_t last_sample_microseconds = time_us_64();
    while(true) {
        sleep_ms(SAMPLE_PERIOD_MILLISECONDS);
        uint64_t now_microseconds = time_us_64();
        uint32_t elapsed_milliseconds = (uint32_t)((now_microseconds - last_sample_microseconds) / 1000u);
        last_sample_microseconds = now_microseconds;

        const uint key_gpios[RemoteInputKeyCount] = {KEY0_GPIO, KEY1_GPIO};
        for(int key = 0; key < (int)RemoteInputKeyCount; key++) {
            RemoteInputOutcome outcome =
                remote_input_model_observe(&input_model, key, key_is_pressed(key_gpios[key]), elapsed_milliseconds);
            if(outcome.kind == RemoteInputOutcomeClassifiedPress) {
                apply_press(&demo, outcome);
            }
        }

        if(demo.refresh_in_progress) {
            if(!panel_is_busy()) {
                note_refresh_finished(&demo);
            }
            continue;
        }
        begin_refresh_if_needed(&demo);
    }
}
