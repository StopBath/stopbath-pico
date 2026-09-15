/*
 * KE1 first light: the image that proves the panel, the two keys and the
 * classification on the device in hand (Pico spec, KE1 hardware gate).
 *
 * It draws a test pattern and a legend, then samples both keys on a fixed
 * tick, feeds the input model, and on each classified press redraws the
 * panel naming the key and the kind, with a count of each. Nothing here
 * talks to anything: no USB, no protocol, no stdio (the SDK's stdio would
 * put line ending translation on the channel the protocol will later use;
 * evaluation log 4.1).
 *
 * Every full refresh blocks for the panel's refresh time, during which no
 * key is sampled. That is acceptable for first light and is exactly what
 * KE2's refresh policy exists to fix.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "hardware/timer.h"
#include "pico/stdlib.h"

#include "../remote_display/remote_bitmap.h"
#include "../remote_display/remote_font.h"
#include "../remote_input/remote_input_model.h"
#include "DEV_Config.h"
#include "keys.h"
#include "panel.h"

/* The legend and log are drawn at this scale; chosen to be readable across
 * a desk, revised in KE2 against the real panel. */
#define TEXT_SCALE 3
#define LINE_HEIGHT (REMOTE_FONT_GLYPH_HEIGHT * TEXT_SCALE + 6)

typedef struct {
    uint32_t counts[RemoteInputKeyCount][RemoteInputPressKindCount];
    char last_event[32];
} FirstLightState;

/* The whole frame, sized once for the life of the program (Flipper 0.10). */
static RemoteBitmap frame;

static const char* key_name(RemoteInputKey key) {
    return key == RemoteInputKey0 ? "KEY0" : "KEY1";
}

static const char* press_kind_name(RemoteInputPressKind press_kind) {
    return press_kind == RemoteInputPressLong ? "LONG" : "SHORT";
}

/* A count as decimal text, without printf: the firmware links no stdio. */
static void write_count(char* destination, size_t capacity, uint32_t count) {
    char reversed[11];
    size_t length = 0;
    do {
        reversed[length++] = (char)('0' + (count % 10));
        count /= 10;
    } while(count > 0 && length < sizeof(reversed));
    size_t written = 0;
    while(length > 0 && written + 1 < capacity) {
        destination[written++] = reversed[--length];
    }
    destination[written] = '\0';
}

static void append(char* destination, size_t capacity, const char* text) {
    size_t used = strlen(destination);
    while(*text != '\0' && used + 1 < capacity) {
        destination[used++] = *text++;
    }
    destination[used] = '\0';
}

static void draw_test_pattern(void) {
    /* A one pixel border proves the extents; a checkerboard block proves
     * pixel level addressing; alternating stripes prove the byte packing
     * survives the driver's transfer. */
    remote_bitmap_fill_rectangle(&frame, 0, 0, REMOTE_BITMAP_WIDTH, 1, RemoteBitmapBlack);
    remote_bitmap_fill_rectangle(&frame, 0, REMOTE_BITMAP_HEIGHT - 1, REMOTE_BITMAP_WIDTH, 1, RemoteBitmapBlack);
    remote_bitmap_fill_rectangle(&frame, 0, 0, 1, REMOTE_BITMAP_HEIGHT, RemoteBitmapBlack);
    remote_bitmap_fill_rectangle(&frame, REMOTE_BITMAP_WIDTH - 1, 0, 1, REMOTE_BITMAP_HEIGHT, RemoteBitmapBlack);

    const int checker_origin_x = 300;
    const int checker_origin_y = 200;
    const int checker_cell = 8;
    for(int row = 0; row < 10; row++) {
        for(int column = 0; column < 10; column++) {
            if((row + column) % 2 == 0) {
                remote_bitmap_fill_rectangle(
                    &frame,
                    checker_origin_x + column * checker_cell,
                    checker_origin_y + row * checker_cell,
                    checker_cell,
                    checker_cell,
                    RemoteBitmapBlack);
            }
        }
    }
    for(int stripe = 0; stripe < 40; stripe += 2) {
        remote_bitmap_fill_rectangle(&frame, 300 + stripe, 150, 1, 40, RemoteBitmapBlack);
    }
}

static void draw_frame(const FirstLightState* state) {
    remote_bitmap_clear(&frame, RemoteBitmapWhite);
    draw_test_pattern();

    int y = 12;
    remote_font_draw_text(&frame, 12, y, "STOPBATH PICO KE1", TEXT_SCALE, RemoteBitmapBlack);
    y += LINE_HEIGHT;
    remote_font_draw_text(&frame, 12, y, panel_driver_name(), TEXT_SCALE, RemoteBitmapBlack);
    y += LINE_HEIGHT * 2;

    remote_font_draw_text(&frame, 12, y, "LAST:", TEXT_SCALE, RemoteBitmapBlack);
    remote_font_draw_text(&frame, 12 + remote_font_text_width("LAST: ", TEXT_SCALE), y, state->last_event, TEXT_SCALE, RemoteBitmapBlack);
    y += LINE_HEIGHT * 2;

    for(int key = 0; key < (int)RemoteInputKeyCount; key++) {
        for(int kind = 0; kind < (int)RemoteInputPressKindCount; kind++) {
            char line[32] = "";
            append(line, sizeof(line), key_name((RemoteInputKey)key));
            append(line, sizeof(line), " ");
            append(line, sizeof(line), press_kind_name((RemoteInputPressKind)kind));
            append(line, sizeof(line), ": ");
            char count_text[12];
            write_count(count_text, sizeof(count_text), state->counts[key][kind]);
            append(line, sizeof(line), count_text);
            remote_font_draw_text(&frame, 12, y, line, TEXT_SCALE, RemoteBitmapBlack);
            y += LINE_HEIGHT;
        }
    }
}

int main(void) {
    keys_initialise();

    DEV_Module_Init();
    panel_initialise();

    FirstLightState state;
    memset(&state, 0, sizeof(state));
    strncpy(state.last_event, "NONE YET", sizeof(state.last_event) - 1);

    RemoteInputModel input_model;
    remote_input_model_initialise(&input_model);

    draw_frame(&state);
    panel_show_full_frame(frame.bytes);

    uint64_t last_sample_microseconds = time_us_64();
    while(true) {
        sleep_ms(KEYS_SAMPLE_PERIOD_MILLISECONDS);
        uint64_t now_microseconds = time_us_64();
        uint32_t elapsed_milliseconds = (uint32_t)((now_microseconds - last_sample_microseconds) / 1000u);
        last_sample_microseconds = now_microseconds;

        bool redraw = false;
        for(int key = 0; key < (int)RemoteInputKeyCount; key++) {
            RemoteInputOutcome outcome =
                remote_input_model_observe(&input_model, key, keys_is_pressed((RemoteInputKey)key), elapsed_milliseconds);
            if(outcome.kind == RemoteInputOutcomeClassifiedPress) {
                state.counts[outcome.key][outcome.press_kind]++;
                state.last_event[0] = '\0';
                append(state.last_event, sizeof(state.last_event), key_name(outcome.key));
                append(state.last_event, sizeof(state.last_event), " ");
                append(state.last_event, sizeof(state.last_event), press_kind_name(outcome.press_kind));
                redraw = true;
            }
        }
        if(redraw) {
            draw_frame(&state);
            panel_show_full_frame(frame.bytes);
            /* The refresh blocked for seconds; the next elapsed figure would
             * otherwise credit that whole time to whichever key is held. */
            last_sample_microseconds = time_us_64();
        }
    }
}
