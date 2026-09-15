# StopBath Pico Remote

A Raspberry Pi Pico 2 W with a Waveshare 4.2 inch e-paper module, acting as a
physical control surface for a StopBath appliance: present the join code,
start a session, end it, without a phone in the photographer's hand, and with
a code large enough for a stranger to scan at arm's length.

The Pico is a peripheral. It reports what physically happened and renders
what it is sent. StopBath remains authoritative for every session, every
guest and the meaning of every button. The specification is
`STOPBATH_PICO_SPEC.md`; it applies `STOPBATH_FLIPPER_SPEC.md` (the first
peripheral) by reference and implements the protocol the StopBath repository
owns in `docs/peripheral/`. What this project asks of the appliance is in
the StopBath repository's `docs/PICO_REMOTE_HANDOFF.md`.

## Status

`KE1` (foundation and first light) is done: the host tests, the typography
scan and the firmware build run on a machine with no Pico attached, and the
author cleared the hardware gate on 2026-09-15 (`HARDWARE_COMPATIBILITY.md`:
the V2 panel driver, both keys, short and long). `KE2` (layouts and the
refresh policy) is done and its gate cleared the same day: every state
judged on the panel, partial refresh about 480 ms with the code still, full
about 1580 ms, no residue. `KE3` (protocol library, session, development
peer) is done. `KE4` (the USB transport) is done on the automated side:
`dist/stopbath_pico.uf2` is the remote itself, speaking protocol version 1
over USB CDC; its hardware gate, twenty cable pulls against the development
peer, is outstanding. Nothing in this repository claims a hardware gate
has passed except where that document records the author saying so.

The Wi-Fi radio is unused. A network transport is a later phase (spec Part 9).

## Hardware

| Item | Value | Source |
|---|---|---|
| Board | Raspberry Pi Pico 2 W (RP2350A, 4 MB flash) | pico-sdk `boards/pico2_w.h` |
| Display | Waveshare Pico-ePaper-4.2, 400 by 300, black and white | Waveshare wiki |
| Panel pins | DIN GP11, CLK GP10, CS GP9, DC GP8, RST GP12, BUSY GP13, on SPI1 | wiki pinout, schematic |
| Keys | KEY0 GP15, KEY1 GP17, to ground, internal pull ups | schematic |

Evidence for each is in `docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md`.

## Building the firmware

Everything is pinned in `scripts/toolchain_versions.env`: pico-sdk `2.3.1`
and the Arm GNU Toolchain `15.2.rel1`. You need `cmake`, `ninja` and `git`
on PATH (on Windows, `scoop install cmake ninja`, and run the scripts from
Git Bash). The setup script downloads the compiler into `.toolchain/` (never
committed), verifies Arm's published digest, and finds or clones the SDK at
the pinned commit.

```bash
bash scripts/setup_toolchain.sh
```

```bash
bash scripts/build_firmware.sh
```

The result is `dist/stopbath_pico.uf2`, the remote; and two check images,
`dist/stopbath_pico_first_light.uf2` (KE1, the panel and keys) and
`dist/stopbath_pico_layout_demo.uf2` (KE2, every display state and the
refresh policy; controls in `HARDWARE_COMPATIBILITY.md`). To flash: hold BOOTSEL
on the Pico, plug it into the PC, release, and copy the `.uf2` onto the
`RP2350` drive that appears. The Pico restarts into it. Use a micro USB cable
that carries data; two of the author's charging cables produced no drive at
all (`HARDWARE_COMPATIBILITY.md`).

## Host tests

The pure logic under `remote_input/` and `remote_display/` has no SDK
dependency and is tested on the development machine with `gcc` and `make`:

```bash
make test
```

The sanitiser build needs a compiler with `libasan` and `libubsan`, which the
MinGW `gcc` on Windows does not ship. Run it under WSL or on Linux:

```bash
make test-sanitise
```

The protocol checks: the generated tables against `protocol.json`, and
`protocol.json` against the appliance's frozen definition:

```bash
make check-protocol-tables check-protocol-definition PYTHON="py -3"
```

The typography scan required by Flipper 0.8:

```bash
make check-typography PYTHON="py -3"
```

## Layout

| Path | Contents |
|---|---|
| `firmware/` | the pico-sdk facing application, kept thin: the remote and the two check programs, the keys, the hardware layer the vendored driver expects, the non-blocking panel driving and refresher, the USB CDC link and descriptors, the build |
| `transport/` | pure logic: the link's decision table (cable and DTR to port opened and closed), no SDK |
| `remote_input/` | pure logic: two keys, debounce, short and long classification, no SDK |
| `remote_display/` | pure logic: the frame buffer in the panel's packing, a bitmap font, the layout with its regions, the fixtures, the refresh policy, no SDK |
| `protocol/` | the parser and encoder copied from the Flipper repository with provenance, and the tables generated from `protocol.json` |
| `session/` | the client session: handshake, replacement, presses to events including the Key1 choice, no SDK |
| `peer/` | the development peer, copied from the Flipper repository with provenance: a host stand-in for the appliance |
| `fuzz/` | the protocol parser fuzz harness |
| `lib/waveshare/` | the vendored panel driver, unmodified, with provenance |
| `tests/` | host tests and the shared harness |
| `scripts/` | the typography scan, the toolchain setup and firmware build scripts, the version pin |
| `docs/evaluation/` | the Plan stage evidence log |

## Documents

`STOPBATH_PICO_SPEC.md`, `PROTOCOL.md`, `TESTING.md`,
`IMPLEMENTATION_DEVIATIONS.md`, `HARDWARE_COMPATIBILITY.md`,
`FIELD_NOTES.md` and
`docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md` are the documents the
specification requires. Each says what it is for at the top.

## Licence

MIT. See `LICENSE`. Code copied from the `stopbath-flipper` repository is
MIT and says where it came from in its header; the vendored Waveshare
drivers carry their own permission notice.
