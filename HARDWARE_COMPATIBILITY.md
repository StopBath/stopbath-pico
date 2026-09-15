# Hardware compatibility

What this firmware has been run on, by whom, and what was observed. Only
the author adds a row that says a gate was cleared. Anything measured on the
device is recorded here with the date; anything not yet measured says so.

## Supported device

| Item | Value | Source |
|---|---|---|
| Board | Raspberry Pi Pico 2 W, RP2350A, 4 MB flash | pico-sdk `src/boards/include/boards/pico2_w.h` at tag `2.3.1` |
| Board marking, as the author read it | `kc r r p2r pico2w` | author, 2026-09-15; it does not identify the panel driver variant |
| Display module | Waveshare Pico-ePaper-4.2, 400 by 300, black and white, 4 grey capable | Waveshare wiki, 2026-09-15 |
| Panel driver variant | `EPD_4in2_V2`; the `EPD_4in2` image drew nothing | author, 2026-09-15 |
| Panel pins | DIN GP11, CLK GP10, CS GP9, DC GP8, RST GP12, BUSY GP13; SPI1 | wiki pinout and schematic `Pico-ePaper-4.2.pdf` |
| Keys | KEY0 on GP15, KEY1 on GP17, each a switch to ground, no board pull up | schematic `Pico-ePaper-4.2.pdf`, sheet 1, "Key" block and Pico pins 20 and 22; on the device not yet reported |
| SDK | pico-sdk `2.3.1`, commit `079c6f39023649b154152db30f1d781e884879bc`, TinyUSB `0.18.0` | `git describe` and `git submodule status` on the author's checkout, 2026-09-15 |
| Compiler | Arm GNU Toolchain `15.2.rel1`, x86_64 hosts | `scripts/toolchain_versions.env` |
| Build tools on the author's machine | cmake `4.4.3`, ninja `1.13.2`, host gcc `15.2.0` (scoop) | checked 2026-09-15 |

## Hardware gates

| Phase | Gate | Status |
|---|---|---|
| `KE1` | the first light image runs on the author's Pico 2 W, draws the pattern and legend, and reports each key press and its kind on the panel; the driver variant and the key GPIOs recorded as observed | PARTLY OBSERVED, NOT CLEARED. The author reported on 2026-09-15 that the V2 image drew and the V1 image did not; the driver variant is settled. Still owed: press each key briefly and hold each for over a second, and confirm the panel names the key and the kind, which settles the key GPIOs. Only the author marks this row cleared. |

## Measurements owed

| Measurement | Owed to | Status |
|---|---|---|
| which driver variant lights the panel | evaluation log 4.3 | observed 2026-09-15: V2 |
| that the keys are on GP15 and GP17 and read low when pressed | evaluation log 4.3 | taken from the schematic, unobserved |
| full refresh time | evaluation log 4.3, `KE2` | manufacturer's 4 s, unmeasured |
| partial refresh time and residue | evaluation log 4.3, `KE2` | unmeasured |
| firmware size against the 4 MB flash | Flipper V1 report | recorded per build in the reproduction report |
| USB identity as Linux sees it | evaluation log 4.2, `KE4` | unobserved |
| total draw beside the appliance's radio adapter | evaluation log 4.5, `KE4` | unmeasured |
