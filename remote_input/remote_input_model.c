#include "remote_input_model.h"

#include <string.h>

static bool key_is_in_range(int key) {
    return key >= 0 && key < (int)RemoteInputKeyCount;
}

static RemoteInputOutcome outcome_of_kind(RemoteInputOutcomeKind kind) {
    RemoteInputOutcome outcome = {.kind = kind, .key = RemoteInputKey0, .press_kind = RemoteInputPressShort};
    return outcome;
}

static RemoteInputOutcome classified(RemoteInputKey key, RemoteInputPressKind press_kind) {
    RemoteInputOutcome outcome = {.kind = RemoteInputOutcomeClassifiedPress, .key = key, .press_kind = press_kind};
    return outcome;
}

void remote_input_model_initialise(RemoteInputModel* input_model) {
    memset(input_model, 0, sizeof(*input_model));
}

static bool any_other_key_is_held(const RemoteInputModel* input_model, int key) {
    for(int other = 0; other < (int)RemoteInputKeyCount; other++) {
        if(other != key && input_model->keys[other].held) {
            return true;
        }
    }
    return false;
}

/* Marks every key currently held, including this one, as overlapped, so a
 * chord ignores both presses whichever is released first. Overlap is sticky
 * for the rest of each hold: once two keys were down together there is no
 * moment at which one of them becomes a clean press again. */
static void mark_held_keys_overlapped(RemoteInputModel* input_model) {
    for(int key = 0; key < (int)RemoteInputKeyCount; key++) {
        if(input_model->keys[key].held) {
            input_model->keys[key].overlapped_another_key = true;
        }
    }
}

static RemoteInputOutcome press_accepted(RemoteInputModel* input_model, int key) {
    RemoteInputKeyState* key_state = &input_model->keys[key];
    key_state->held = true;
    key_state->held_milliseconds = 0;
    key_state->long_press_reported = false;
    key_state->overlapped_another_key = false;
    if(any_other_key_is_held(input_model, key)) {
        mark_held_keys_overlapped(input_model);
    }
    return outcome_of_kind(RemoteInputOutcomeNothing);
}

static RemoteInputOutcome release_accepted(RemoteInputModel* input_model, int key) {
    RemoteInputKeyState* key_state = &input_model->keys[key];
    bool was_overlapped = key_state->overlapped_another_key;
    bool long_already_reported = key_state->long_press_reported;
    key_state->held = false;
    key_state->held_milliseconds = 0;
    key_state->long_press_reported = false;
    key_state->overlapped_another_key = false;
    if(was_overlapped) {
        return outcome_of_kind(RemoteInputOutcomeIgnored);
    }
    if(long_already_reported) {
        /* The long press went out when the threshold was reached; the
         * release completes nothing new. */
        return outcome_of_kind(RemoteInputOutcomeNothing);
    }
    return classified((RemoteInputKey)key, RemoteInputPressShort);
}

static RemoteInputOutcome held_advanced(RemoteInputModel* input_model, int key, uint32_t elapsed_milliseconds) {
    RemoteInputKeyState* key_state = &input_model->keys[key];
    /* Saturate rather than wrap: a key held for 49 days must not become a
     * short press. */
    if(UINT32_MAX - key_state->held_milliseconds < elapsed_milliseconds) {
        key_state->held_milliseconds = UINT32_MAX;
    } else {
        key_state->held_milliseconds += elapsed_milliseconds;
    }
    if(key_state->held_milliseconds >= REMOTE_INPUT_LONG_PRESS_THRESHOLD_MILLISECONDS &&
       !key_state->long_press_reported && !key_state->overlapped_another_key) {
        key_state->long_press_reported = true;
        return classified((RemoteInputKey)key, RemoteInputPressLong);
    }
    return outcome_of_kind(RemoteInputOutcomeNothing);
}

RemoteInputOutcome remote_input_model_observe(
    RemoteInputModel* input_model,
    int key,
    bool pressed_level,
    uint32_t elapsed_milliseconds) {
    if(!key_is_in_range(key)) {
        return outcome_of_kind(RemoteInputOutcomeIgnored);
    }
    RemoteInputKeyState* key_state = &input_model->keys[key];

    /* Debounce: a level that disagrees with the believed state must persist
     * for the whole interval before it is believed. Time spent settling is
     * not counted as hold time, so the thresholds are measured from the
     * moment the press is believed, which is what a person feels. */
    if(pressed_level == key_state->held) {
        key_state->raw_level = pressed_level;
        key_state->disagreement_milliseconds = 0;
        return key_state->held ? held_advanced(input_model, key, elapsed_milliseconds)
                               : outcome_of_kind(RemoteInputOutcomeNothing);
    }

    if(pressed_level != key_state->raw_level) {
        key_state->raw_level = pressed_level;
        key_state->disagreement_milliseconds = 0;
    }
    key_state->disagreement_milliseconds += elapsed_milliseconds;
    if(key_state->disagreement_milliseconds < REMOTE_INPUT_DEBOUNCE_MILLISECONDS) {
        /* Still settling. Hold time is not advanced while the level reads
         * released, so a hold one tick under the threshold cannot become a
         * long press during its own release; a bounce up mid hold merely
         * pauses the clock for a few milliseconds. */
        return outcome_of_kind(RemoteInputOutcomeNothing);
    }

    key_state->disagreement_milliseconds = 0;
    return pressed_level ? press_accepted(input_model, key) : release_accepted(input_model, key);
}

bool remote_input_model_is_key_held(const RemoteInputModel* input_model, int key) {
    if(!key_is_in_range(key)) {
        return false;
    }
    return input_model->keys[key].held;
}
