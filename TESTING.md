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
| `tests/test_remote_display_layout.c` (KE2) | host, any `gcc` | `make test` |
| `tests/test_remote_refresh_policy.c` (KE2) | host, any `gcc` | `make test` |
| `tests/test_remote_protocol.c` (KE3, copied from the Flipper repository) | host, any `gcc` | `make test` |
| `tests/test_development_peer.c` (KE3, copied from the Flipper repository) | host, any `gcc` | `make test` |
| `tests/test_remote_session.c` (KE3) | host, any `gcc` | `make test` |
| `tests/test_remote_link_edge.c` (KE4) | host, any `gcc` | `make test` |
| protocol parser fuzz harness (KE3) | host, deterministic; sanitised on Linux | `make fuzz`, `make fuzz-sanitise` |
| generated tables match `protocol.json` (KE3) | any Python 3 | `make check-protocol-tables` |
| `protocol.json` is the appliance's frozen definition (KE3) | any Python 3 | `make check-protocol-definition` |
| the development peer builds (KE3) | Linux or WSL (termios) | `make peer` |
| a first look at the link from Windows (KE4, a hardware check, not a test) | Windows PowerShell, the device on a COM port | `scripts\link_check.ps1 -Port COMn` |
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

`KE2`: every fixture renders; the regions are byte aligned, inside the
panel and disjoint; an unknown status or page renders the fallback; long
values are cut inside their region; each field change maps to its regions
and the rendered pixel difference is confined to them; the code region is
identical across every pair of fixtures sharing a page and payload; a link
screen change reports every region; rendering never allocates. And the
policy: no change is no refresh, a count, error or status change is partial
and never touches the code, a page, payload or link change is full, the
forced full arrives at the bound and any full resets it. In
`tests/test_remote_display_layout.c` and `tests/test_remote_refresh_policy.c`,
written and seen to fail (no rule to make the target) before the modules
existed.

`KE3`: the protocol library's own suite, copied with it (every verb round
trips, truncated, overlong, unknown verb, unknown field, missing field, wrong
version, embedded null, bounds at and over the limit, no allocation in the
parse path), and the session's: HELLO carries version 1, the token and
`locked=0`; the first record is the acceptance; a later record replaces the
previous wholly; an error code shows as its wire name; `BAD_VERSION` marks
the link incompatible; closing the port discards everything including an
unsent press and clears the payload; a line cut by a disconnection is not
completed after reconnection; a press is sent only while connected and
otherwise dropped and counted; every press carries both guard flags true;
Key1 long sends nothing; the Key1 mapping can only produce a page event for
any page value; the output queue is bounded and overflow is counted; a lost
handshake is retried on the interval; malformed input is counted and leaves
the record alone; the session never allocates; and, owed by KE2, every
error code the protocol defines renders inside the error band. In
`tests/test_remote_session.c`, written and seen to fail before the session
existed.

`KE4`: the transport glue's decision table is total over every prior state
and observation, opens only on cable and DTR together, reports each edge
once, and treats a cable pull and reinsertion as a close then an open, the
same as the Flipper's transport. In `tests/test_remote_link_edge.c`, written
and seen to fail before the module existed. The SDK facing edge
(`firmware/usb_link.c`) is not testable off the device and is the `KE4`
hardware gate.

## Testing deviations

Hardware specific behaviour that cannot reasonably be automated, with the
reason (Flipper Part 6):

| Behaviour | Why not automated | Where it is checked |
|---|---|---|
| The panel accepts the frame and shows it | needs the panel; the packing is proven on the host, the transfer is not | `KE1` gate |
| Which vendored driver the panel in hand needs | needs the panel | `KE1` gate; observed 2026-09-15, V2 |
| The keys are on GP15 and GP17 and read low when pressed | needs the board; taken from the schematic | `KE1` gate |
| The classification thresholds feel right | a hand, not a test | `KD11`, field use |
| Partial refresh leaves the code region undisturbed on the glass and how much residue it leaves | needs the panel; the bitmap side is proven on the host | `KE2` gate |
| Full and partial refresh durations | needs the panel; the demo measures and shows them | `KE2` gate |
| The USB link opens on DTR, closes on a cable pull, and recovers | needs the device and a host; the decision table is proven on the host, TinyUSB's reporting of the facts is not | `KE4` gate |

## Hardware gates

Cleared only by the author on the device, recorded in
`HARDWARE_COMPATIBILITY.md`. Nothing here claims one has passed.
