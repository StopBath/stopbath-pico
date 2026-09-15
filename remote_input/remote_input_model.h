/*
 * The input model: what the remote makes of its two keys.
 *
 * No SDK dependency, so it builds and tests on a development machine (Pico
 * spec Part 3). The firmware samples each key's level on a fixed tick and
 * feeds every sample here; the model debounces, times the hold, and says
 * when a press has been classified as short or long. It assigns no meaning
 * to a press: which protocol event a press becomes is the session's business
 * (spec 2.2, 2.3), and what that event means is the appliance's (Flipper 2.1).
 *
 * Unlike the Flipper, whose firmware classified presses for it, the Pico has
 * only raw levels, which is why the thresholds live here (Appendix A gives
 * them as starting points; KD11 settles them from field use).
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A hold at or over this is a long press; under it is short. The main
 * specification's 5.4 guidance for hold to terminate is about one second. */
#define REMOTE_INPUT_LONG_PRESS_THRESHOLD_MILLISECONDS 1000

/* A level must hold this long before it is believed. Mechanical keys bounce
 * for a few milliseconds; twenty covers that without making a tap feel slow.
 * A bounce shorter than this is neither a press nor a release. */
#define REMOTE_INPUT_DEBOUNCE_MILLISECONDS 20

/* The physical keys, named for their position on the module (KEY0, KEY1 on
 * the silkscreen), not for any meaning. */
typedef enum {
    RemoteInputKey0,
    RemoteInputKey1,
    RemoteInputKeyCount,
} RemoteInputKey;

typedef enum {
    RemoteInputPressShort,
    RemoteInputPressLong,
    RemoteInputPressKindCount,
} RemoteInputPressKind;

typedef enum {
    /* This observation completed nothing: the key is idle, still held, or
     * still settling through the debounce interval. */
    RemoteInputOutcomeNothing,
    /* A press completed but is not reported: it overlapped another key, or
     * the key index was outside the enumeration. The model is unchanged in
     * the second case. */
    RemoteInputOutcomeIgnored,
    /* A press was classified; key and press_kind are valid. A short press is
     * reported on release, a long press the moment the threshold is reached
     * while still held, so the release that follows reports nothing. */
    RemoteInputOutcomeClassifiedPress,
} RemoteInputOutcomeKind;

typedef struct {
    RemoteInputOutcomeKind kind;
    /* Valid only when kind is RemoteInputOutcomeClassifiedPress. */
    RemoteInputKey key;
    RemoteInputPressKind press_kind;
} RemoteInputOutcome;

typedef struct {
    /* The level the model believes, after debouncing. */
    bool held;
    /* The level last observed, and how long it has disagreed with held. */
    bool raw_level;
    uint32_t disagreement_milliseconds;
    /* How long the key has been believed held. */
    uint32_t held_milliseconds;
    /* Set once the long press has been reported for this hold. */
    bool long_press_reported;
    /* Set when this hold overlapped another key's hold; the press is then
     * ignored on release, and no long press is reported for it. */
    bool overlapped_another_key;
} RemoteInputKeyState;

typedef struct {
    RemoteInputKeyState keys[RemoteInputKeyCount];
} RemoteInputModel;

/* Every key idle and believed released. */
void remote_input_model_initialise(RemoteInputModel* input_model);

/* One sample of one key: whether the key reads as pressed now, and how long
 * it has been since this key was last observed. Takes a plain integer for
 * the key so an out of range value can be passed without undefined behaviour
 * on the enum; such a value is ignored. */
RemoteInputOutcome remote_input_model_observe(
    RemoteInputModel* input_model,
    int key,
    bool pressed_level,
    uint32_t elapsed_milliseconds);

/* The debounced state of a key. An out of range key reads as not held. */
bool remote_input_model_is_key_held(const RemoteInputModel* input_model, int key);

#ifdef __cplusplus
}
#endif
