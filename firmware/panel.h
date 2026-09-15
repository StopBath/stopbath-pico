/*
 * The panel behind one interface, so nothing above it names a Waveshare
 * function. panel.c binds the vendored EPD_4in2_V2 driver, the one the
 * author observed lighting the panel in hand on 2026-09-15 (evaluation log
 * 4.3); the alternative driver was tried first and drew nothing.
 */
#pragma once

#include <stdint.h>

/* Resets and initialises the controller for a full refresh. Blocks on the
 * panel's busy line. */
void panel_initialise(void);

/* Sends one full frame in the packing remote_bitmap produces and refreshes
 * the whole panel. Blocks for the refresh (about four seconds, unmeasured). */
void panel_show_full_frame(const uint8_t* frame);

/* Deep sleep. The manufacturer advises sleeping between refreshes; waking
 * needs panel_initialise again. */
void panel_sleep(void);

/* The vendored driver this build binds, for the screen and the log. */
const char* panel_driver_name(void);
