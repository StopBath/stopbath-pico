/*
 * KE1 tests first (Pico spec, KE1): key classification and debounce.
 *
 * The Pico has no firmware input layer to classify a press, so the model does
 * it from sampled levels and elapsed time. These tests drive it the way the
 * firmware's loop will: one observation per key per tick, with the tick's
 * length.
 */
#include "test_support.h"

#include "../remote_input/remote_input_model.h"

/* The firmware samples on a fixed tick; tests use a round figure that
 * divides the thresholds so the arithmetic in each case is readable. */
#define TICK_MILLISECONDS 10

static RemoteInputModel fresh_model(void) {
    RemoteInputModel input_model;
    remote_input_model_initialise(&input_model);
    return input_model;
}

/* Holds a key at a level for a duration, one tick at a time, returning the
 * last outcome that was not Nothing (or Nothing if there was none). */
static RemoteInputOutcome hold_level(
    RemoteInputModel* input_model,
    int key,
    bool pressed_level,
    uint32_t duration_milliseconds) {
    RemoteInputOutcome last_outcome = {.kind = RemoteInputOutcomeNothing};
    for(uint32_t elapsed = 0; elapsed < duration_milliseconds; elapsed += TICK_MILLISECONDS) {
        RemoteInputOutcome outcome =
            remote_input_model_observe(input_model, key, pressed_level, TICK_MILLISECONDS);
        if(outcome.kind != RemoteInputOutcomeNothing) {
            last_outcome = outcome;
        }
    }
    return last_outcome;
}

/* A press: debounced down, held, debounced up. Returns the outcome seen at
 * any point during the press, which is where a long press is reported. */
static RemoteInputOutcome press_for(RemoteInputModel* input_model, int key, uint32_t held_milliseconds) {
    RemoteInputOutcome during = hold_level(
        input_model, key, true, REMOTE_INPUT_DEBOUNCE_MILLISECONDS + held_milliseconds);
    RemoteInputOutcome on_release =
        hold_level(input_model, key, false, REMOTE_INPUT_DEBOUNCE_MILLISECONDS + TICK_MILLISECONDS);
    return on_release.kind != RemoteInputOutcomeNothing ? on_release : during;
}

typedef struct {
    int key;
    uint32_t held_milliseconds;
    RemoteInputPressKind expected_kind;
    const char* description;
} ClassificationRow;

static const ClassificationRow classification_rows[] = {
    {RemoteInputKey0, TICK_MILLISECONDS * 5, RemoteInputPressShort, "key0 tap is short"},
    {RemoteInputKey1, TICK_MILLISECONDS * 5, RemoteInputPressShort, "key1 tap is short"},
    {RemoteInputKey0,
     REMOTE_INPUT_LONG_PRESS_THRESHOLD_MILLISECONDS - TICK_MILLISECONDS,
     RemoteInputPressShort,
     "key0 one tick under the threshold is short"},
    {RemoteInputKey0,
     REMOTE_INPUT_LONG_PRESS_THRESHOLD_MILLISECONDS,
     RemoteInputPressLong,
     "key0 exactly at the threshold is long"},
    {RemoteInputKey1,
     REMOTE_INPUT_LONG_PRESS_THRESHOLD_MILLISECONDS * 3,
     RemoteInputPressLong,
     "key1 far over the threshold is long"},
};

static void a_hold_under_the_threshold_is_short_and_at_or_over_it_is_long(RemoteTestReport* report) {
    for(int row_index = 0; row_index < REMOTE_TEST_ROW_COUNT(classification_rows); row_index++) {
        const ClassificationRow* row = &classification_rows[row_index];
        RemoteInputModel input_model = fresh_model();
        RemoteInputOutcome outcome = press_for(&input_model, row->key, row->held_milliseconds);
        REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteInputOutcomeClassifiedPress, outcome.kind, row->description);
        REMOTE_TEST_ASSERT_EQUAL_INT(report, row->key, outcome.key, row->description);
        REMOTE_TEST_ASSERT_EQUAL_INT(report, row->expected_kind, outcome.press_kind, row->description);
    }
}

/* A long press is reported the moment the threshold is reached, while the
 * key is still held, so the photographer does not have to time a release;
 * the release that follows then reports nothing. */
static void a_long_press_is_reported_while_still_held_and_its_release_reports_nothing(RemoteTestReport* report) {
    RemoteInputModel input_model = fresh_model();
    RemoteInputOutcome during = hold_level(
        &input_model,
        RemoteInputKey0,
        true,
        REMOTE_INPUT_DEBOUNCE_MILLISECONDS + REMOTE_INPUT_LONG_PRESS_THRESHOLD_MILLISECONDS);
    REMOTE_TEST_ASSERT_EQUAL_INT(
        report, RemoteInputOutcomeClassifiedPress, during.kind, "long reported before release");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteInputPressLong, during.press_kind, "and it is long");

    RemoteInputOutcome still_held = hold_level(
        &input_model, RemoteInputKey0, true, REMOTE_INPUT_LONG_PRESS_THRESHOLD_MILLISECONDS);
    REMOTE_TEST_ASSERT_EQUAL_INT(
        report, RemoteInputOutcomeNothing, still_held.kind, "holding on reports the long press once only");

    RemoteInputOutcome on_release =
        hold_level(&input_model, RemoteInputKey0, false, REMOTE_INPUT_DEBOUNCE_MILLISECONDS * 2);
    REMOTE_TEST_ASSERT_EQUAL_INT(
        report, RemoteInputOutcomeNothing, on_release.kind, "the release after a long press is silent");
}

/* A bounce shorter than the debounce interval, in either direction, is not a
 * press and not a release. */
static void a_bounce_shorter_than_the_debounce_interval_is_not_a_press(RemoteTestReport* report) {
    RemoteInputModel input_model = fresh_model();
    RemoteInputOutcome bounce_down = hold_level(
        &input_model, RemoteInputKey0, true, REMOTE_INPUT_DEBOUNCE_MILLISECONDS - TICK_MILLISECONDS);
    RemoteInputOutcome back_up =
        hold_level(&input_model, RemoteInputKey0, false, REMOTE_INPUT_DEBOUNCE_MILLISECONDS * 2);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteInputOutcomeNothing, bounce_down.kind, "a bounce down is nothing");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteInputOutcomeNothing, back_up.kind, "and returning up is nothing");
    REMOTE_TEST_ASSERT(report, !remote_input_model_is_key_held(&input_model, RemoteInputKey0), "not held");

    /* A real press, then a bounce up mid hold, must not end the press. */
    hold_level(&input_model, RemoteInputKey0, true, REMOTE_INPUT_DEBOUNCE_MILLISECONDS + TICK_MILLISECONDS);
    REMOTE_TEST_ASSERT(report, remote_input_model_is_key_held(&input_model, RemoteInputKey0), "held after debounce");
    RemoteInputOutcome bounce_up = hold_level(
        &input_model, RemoteInputKey0, false, REMOTE_INPUT_DEBOUNCE_MILLISECONDS - TICK_MILLISECONDS);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteInputOutcomeNothing, bounce_up.kind, "a bounce up mid hold is nothing");
    REMOTE_TEST_ASSERT(report, remote_input_model_is_key_held(&input_model, RemoteInputKey0), "still held");
}

/* Two keys down at once is not a gesture the table names. Neither key
 * reports, whichever is released first, and nothing is left over for the
 * next press. */
static void a_press_overlapping_another_key_is_ignored_on_both_keys(RemoteTestReport* report) {
    RemoteInputModel input_model = fresh_model();
    hold_level(&input_model, RemoteInputKey0, true, REMOTE_INPUT_DEBOUNCE_MILLISECONDS + TICK_MILLISECONDS);
    hold_level(&input_model, RemoteInputKey1, true, REMOTE_INPUT_DEBOUNCE_MILLISECONDS + TICK_MILLISECONDS);

    RemoteInputOutcome key0_release =
        hold_level(&input_model, RemoteInputKey0, false, REMOTE_INPUT_DEBOUNCE_MILLISECONDS * 2);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteInputOutcomeIgnored, key0_release.kind, "key0 chord is ignored");

    RemoteInputOutcome key1_release =
        hold_level(&input_model, RemoteInputKey1, false, REMOTE_INPUT_DEBOUNCE_MILLISECONDS * 2);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteInputOutcomeIgnored, key1_release.kind, "key1 chord is ignored");

    /* Overlap that runs past the long threshold reports no long press either. */
    hold_level(&input_model, RemoteInputKey0, true, REMOTE_INPUT_DEBOUNCE_MILLISECONDS + TICK_MILLISECONDS);
    hold_level(&input_model, RemoteInputKey1, true, REMOTE_INPUT_DEBOUNCE_MILLISECONDS + TICK_MILLISECONDS);
    RemoteInputOutcome long_while_chorded = hold_level(
        &input_model, RemoteInputKey0, true, REMOTE_INPUT_LONG_PRESS_THRESHOLD_MILLISECONDS * 2);
    REMOTE_TEST_ASSERT_EQUAL_INT(
        report, RemoteInputOutcomeNothing, long_while_chorded.kind, "no long press while chorded");
    hold_level(&input_model, RemoteInputKey0, false, REMOTE_INPUT_DEBOUNCE_MILLISECONDS * 2);
    hold_level(&input_model, RemoteInputKey1, false, REMOTE_INPUT_DEBOUNCE_MILLISECONDS * 2);

    /* And a clean press afterwards is classified normally. */
    RemoteInputOutcome clean = press_for(&input_model, RemoteInputKey1, TICK_MILLISECONDS * 5);
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteInputOutcomeClassifiedPress, clean.kind, "clean press after a chord");
    REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteInputPressShort, clean.press_kind, "is short");
}

/* A key index outside the enumeration, from a corrupt or future input value,
 * is ignored and changes nothing. Plain integers so the test can pass one
 * without undefined behaviour on the enum. */
static void an_out_of_range_key_is_ignored_and_changes_nothing(RemoteTestReport* report) {
    const int out_of_range_keys[] = {-1, RemoteInputKeyCount, RemoteInputKeyCount + 7, 1000};
    for(int row_index = 0; row_index < REMOTE_TEST_ROW_COUNT(out_of_range_keys); row_index++) {
        RemoteInputModel input_model = fresh_model();
        RemoteInputOutcome outcome = hold_level(
            &input_model,
            out_of_range_keys[row_index],
            true,
            REMOTE_INPUT_DEBOUNCE_MILLISECONDS + REMOTE_INPUT_LONG_PRESS_THRESHOLD_MILLISECONDS);
        REMOTE_TEST_ASSERT_EQUAL_INT(report, RemoteInputOutcomeIgnored, outcome.kind, "out of range key is ignored");
        REMOTE_TEST_ASSERT(report, !remote_input_model_is_key_held(&input_model, RemoteInputKey0), "key0 untouched");
        REMOTE_TEST_ASSERT(report, !remote_input_model_is_key_held(&input_model, RemoteInputKey1), "key1 untouched");
    }
}

/* The input path is sampled every tick for the life of the device; it must
 * never allocate (Flipper 0.10). */
static void observing_never_allocates(RemoteTestReport* report) {
    RemoteInputModel input_model = fresh_model();
    int allocations_before = remote_test_allocation_count;
    press_for(&input_model, RemoteInputKey0, REMOTE_INPUT_LONG_PRESS_THRESHOLD_MILLISECONDS);
    press_for(&input_model, RemoteInputKey1, TICK_MILLISECONDS);
    REMOTE_TEST_ASSERT_EQUAL_INT(
        report, allocations_before, remote_test_allocation_count, "no allocation in the input path");
}

static const RemoteTestCase test_cases[] = {
    {"a hold under the threshold is short and at or over it is long",
     a_hold_under_the_threshold_is_short_and_at_or_over_it_is_long},
    {"a long press is reported while still held and its release reports nothing",
     a_long_press_is_reported_while_still_held_and_its_release_reports_nothing},
    {"a bounce shorter than the debounce interval is not a press",
     a_bounce_shorter_than_the_debounce_interval_is_not_a_press},
    {"a press overlapping another key is ignored on both keys",
     a_press_overlapping_another_key_is_ignored_on_both_keys},
    {"an out of range key is ignored and changes nothing", an_out_of_range_key_is_ignored_and_changes_nothing},
    {"observing never allocates", observing_never_allocates},
};

int main(void) {
    return remote_test_run_all(test_cases, REMOTE_TEST_ROW_COUNT(test_cases));
}
