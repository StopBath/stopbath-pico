/*
 * The two keys as GPIO. From the module schematic (evaluation log 4.3),
 * confirmed on the device at the KE1 gate: each is a switch to ground with no
 * pull up on the board, so the Pico's internal pull ups are enabled and a
 * press reads low. Shared by every program so the pins live in one place.
 */
#pragma once

#include <stdbool.h>

#include "../remote_input/remote_input_model.h"

/* Sampling period for the input model. Well under the debounce interval so
 * a bounce is seen as several samples, and fast enough that the long press
 * threshold lands within a few milliseconds of the figure. */
#define KEYS_SAMPLE_PERIOD_MILLISECONDS 5u

void keys_initialise(void);
bool keys_is_pressed(RemoteInputKey key);
