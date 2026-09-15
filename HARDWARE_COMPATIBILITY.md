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
| Keys | KEY0 on GP15, KEY1 on GP17, each a switch to ground, no board pull up | schematic `Pico-ePaper-4.2.pdf`, sheet 1, "Key" block and Pico pins 20 and 22; confirmed on the device by the author, 2026-09-15 |
| SDK | pico-sdk `2.3.1`, commit `079c6f39023649b154152db30f1d781e884879bc`, TinyUSB `0.18.0` | `git describe` and `git submodule status` on the author's checkout, 2026-09-15 |
| Compiler | Arm GNU Toolchain `15.2.rel1`, x86_64 hosts | `scripts/toolchain_versions.env` |
| Build tools on the author's machine | cmake `4.4.3`, ninja `1.13.2`, host gcc `15.2.0` (scoop) | checked 2026-09-15 |

## Hardware gates

| Phase | Gate | Status |
|---|---|---|
| `KE1` | the first light image runs on the author's Pico 2 W, draws the pattern and legend, and reports each key press and its kind on the panel; the driver variant and the key GPIOs recorded as observed | CLEARED 2026-09-15 by the author: the V2 image drew and the V1 image did not, settling the driver variant; then "Key0 and Key1 both update on short and long", settling the key GPIOs and the classification on the device. Observed alongside, and expected of first light: a press made while the panel is refreshing is not recorded, because the program samples no key during the blocking full refresh. That is the limitation KE2's refresh policy exists to remove (spec 2.6). Not recorded: whether the panel was read in daylight, and the refresh time, which KE2 measures. |

| `KE2` | each display state legible on the panel at arm's length in daylight (`KD8`); a partial refresh of the count region does not disturb the code region, observed; residue after the measured number of partials is acceptable or the forced full interval is set accordingly | NOT YET CLEARED. Flash `dist/stopbath_pico_layout_demo.uf2`. KEY1 short steps through the fixtures (a full refresh each): judge every one. On a wifi or gallery fixture, press KEY0 short several times and watch the count change while the code frame stays still; hold KEY0 to toggle the error band the same way. KEY1 long shows the diagnostic screen with the last refresh's duration in milliseconds and the running counts; read off a full refresh time and a partial refresh time. Report: legibility, whether the code frame moved during a partial, how the residue looks after five partials, and both durations. |

## Measurements owed

| Measurement | Owed to | Status |
|---|---|---|
| which driver variant lights the panel | evaluation log 4.3 | observed 2026-09-15: V2 |
| that the keys are on GP15 and GP17 and read low when pressed | evaluation log 4.3 | observed 2026-09-15: both keys report short and long |
| full refresh time | evaluation log 4.3, `KE2` | manufacturer's 4 s; the layout demo shows the measured figure |
| partial refresh time and residue | evaluation log 4.3, `KE2` | the layout demo shows the time; residue is judged by eye |
| firmware size against the 4 MB flash | Flipper V1 report | recorded per build in the reproduction report |
| USB identity as Linux sees it | evaluation log 4.2, `KE4` | unobserved |
| total draw beside the appliance's radio adapter | evaluation log 4.5, `KE4` | unmeasured |
