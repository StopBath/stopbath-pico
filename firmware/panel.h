/*
 * The panel behind one interface, so nothing above it names a Waveshare
 * function. panel.c drives the EPD_4in2_V2 controller, the one the author
 * observed lighting the panel in hand on 2026-09-15 (evaluation log 4.3).
 *
 * Refreshes are started, not waited for. The vendored driver's display
 * calls spin on the BUSY line for the whole refresh, during which first
 * light sampled no key and the author lost presses (KE1 gate). So the
 * update is begun here and the main loop polls panel_is_busy between key
 * samples; the command sequences are the vendored driver's, cited line by
 * line in panel.c, sent through the same DEV_ primitives.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Resets and initialises the controller for full refreshes. Blocks on the
 * busy line for the reset, a few hundred milliseconds, at start only. */
void panel_initialise(void);

/* Sends the whole frame (remote_bitmap packing) and starts a full refresh.
 * Returns once the bytes are sent; the refresh runs on. Restores the full
 * refresh registers a partial pass changes, so the two can alternate. */
void panel_begin_full_refresh(const uint8_t* frame);

/* Sends one rectangle of the frame and starts a partial refresh of it. The
 * rectangle's x and width must be multiples of eight, because the
 * controller addresses RAM by the byte; the caller (the layout's regions)
 * guarantees it. */
void panel_begin_partial_refresh(const uint8_t* frame, int x, int y, int width, int height);

/* Whether a refresh is still running. */
bool panel_is_busy(void);

/* Begins a full refresh and waits for it. For first light and for the
 * initial draw, where nothing else is happening. */
void panel_show_full_frame(const uint8_t* frame);

/* Deep sleep. The manufacturer advises sleeping between refreshes; waking
 * needs panel_initialise again. */
void panel_sleep(void);

/* The vendored driver this build binds, for the screen and the log. */
const char* panel_driver_name(void);
