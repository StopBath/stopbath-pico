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
`docs/APPLIANCE_HANDOFF.md`.

## Status

`KE1` (foundation and first light) is done on the automated side. The host
tests, the typography scan and the firmware build run on a machine with no
Pico attached. The `KE1` hardware gate, the first light image on the author's
device, is the author's to clear and is recorded in
`HARDWARE_COMPATIBILITY.md`. Nothing in this repository claims a hardware
gate has passed.

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
scripts/setup_toolchain.sh
```

```bash
scripts/build_firmware.sh
```

The result is `dist/stopbath_pico_first_light_v1.uf2` and
`dist/stopbath_pico_first_light_v2.uf2`, one per vendored panel driver
(`lib/waveshare/PROVENANCE.md` says why there are two). To flash: hold
BOOTSEL, plug the Pico in, release, and copy one `.uf2` onto the drive that
appears. The Pico restarts into it.

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

The typography scan required by Flipper 0.8:

```bash
make check-typography PYTHON="py -3"
```

## Layout

| Path | Contents |
|---|---|
| `firmware/` | the pico-sdk facing application, kept thin: the first light program, the hardware layer the vendored drivers expect, the panel bindings, the build |
| `remote_input/` | pure logic: two keys, debounce, short and long classification, no SDK |
| `remote_display/` | pure logic: the frame buffer in the panel's packing, a bitmap font, no SDK |
| `lib/waveshare/` | the vendored panel drivers, unmodified, with provenance |
| `tests/` | host tests and the shared harness |
| `scripts/` | the typography scan, the toolchain setup and firmware build scripts, the version pin |
| `docs/evaluation/` | the Plan stage evidence log |
| `docs/APPLIANCE_HANDOFF.md` | what the StopBath repository is asked to change |

## Documents

`STOPBATH_PICO_SPEC.md`, `PROTOCOL.md`, `TESTING.md`,
`IMPLEMENTATION_DEVIATIONS.md`, `HARDWARE_COMPATIBILITY.md`,
`FIELD_NOTES.md`, `docs/APPLIANCE_HANDOFF.md` and
`docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md` are the documents the
specification requires. Each says what it is for at the top.

## Licence

MIT. See `LICENSE`. Code copied from the `stopbath-flipper` repository is
MIT and says where it came from in its header; the vendored Waveshare
drivers carry their own permission notice.
