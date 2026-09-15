# Testing

Tests cover behaviour, not implementation trivia (Flipper Part 6, applied by
Pico spec Part 6). What is automated runs on a development machine with no
Pico attached and no SDK present. What cannot be automated is listed here as
a hardware gate or as a testing deviation with its reason, never left
unstated.

## Automated

| Suite | Runs where | Command |
|---|---|---|
| `tests/test_remote_input_model.c` (KE1) | host, any `gcc` | `make test` |
| `tests/test_remote_bitmap.c` (KE1) | host, any `gcc` | `make test` |
| all of the above under address and undefined behaviour sanitisers | Linux or WSL | `make test-sanitise` |
| typography scan (Flipper 0.8) | any Python 3 | `make check-typography` |
| firmware build, warnings as errors, against the pinned SDK and compiler | host with the toolchain from `scripts/setup_toolchain.sh` | `scripts/build_firmware.sh` |

Every unit test is a table driven C function run by the shared harness in
`tests/test_support.h`, copied from the Flipper repository. A test case with
no assertions fails; an empty test proves nothing. Every test binary is
linked with the heap functions wrapped, so a test can prove the code under it
did not allocate.

Continuous integration (`.github/workflows/ci.yml`) runs all of the above on
every pull request and on every push to `main`. The firmware job runs the
same two scripts the development machine runs.

## What each phase tests first

`KE1`: key classification (a hold under the threshold is short, at or over it
is long, a long press is reported while still held), debounce (a bounce
shorter than the interval is neither a press nor a release), a press
overlapping another key is ignored on both keys, an out of range key is
ignored, and the input path never allocates. All in
`tests/test_remote_input_model.c`, written and seen to fail (no rule to make
the target, the module being absent) before the model was implemented.

Also `KE1`, because a first light image drawn into the wrong packing looks
like noise: the frame buffer's size, its bit order (leftmost pixel in the
most significant bit, 1 white), row addressing, clipping of pixels and
rectangles, and the font's cell geometry, scaling, unknown character
handling and case folding. All in `tests/test_remote_bitmap.c`.

## Testing deviations

Hardware specific behaviour that cannot reasonably be automated, with the
reason (Flipper Part 6):

| Behaviour | Why not automated | Where it is checked |
|---|---|---|
| The panel accepts the frame and shows it | needs the panel; the packing is proven on the host, the transfer is not | `KE1` gate |
| Which vendored driver the panel in hand needs | needs the panel | `KE1` gate; observed 2026-09-15, V2 |
| The keys are on GP15 and GP17 and read low when pressed | needs the board; taken from the schematic | `KE1` gate |
| The classification thresholds feel right | a hand, not a test | `KD11`, field use |

## Hardware gates

Cleared only by the author on the device, recorded in
`HARDWARE_COMPATIBILITY.md`. Nothing here claims one has passed.
