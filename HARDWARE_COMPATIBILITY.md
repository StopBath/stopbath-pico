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

| `KE2` | each display state legible on the panel at arm's length in daylight (`KD8`); a partial refresh of the count region does not disturb the code region, observed; residue after the measured number of partials is acceptable or the forced full interval is set accordingly | CLEARED 2026-09-15 by the author: every fixture legible ("everything else looks good"), the layouts accepted as KD8; partial refreshes of the count region and the error band land with the code frame still; "no ghosting or residual at all" after four and after twenty four partials; partial refresh about 480 ms, full about 1580 ms, from the diagnostic screen. The forced full bound moved from 5 to 100. Not recorded: daylight specifically; indoor legibility was judged. |

| `KE4` | against the development peer: pull and reinsert the cable twenty times and restart each side independently, with no repair step; the USB identity the Pico presents recorded | CLEARED 2026-09-15 by the author. On the Windows PC (`scripts/link_check.ps1 -Port COM8`): HELLO on open, retried at two seconds, silenced by the DISPLAY acceptance; presses arrived with both guard flags true; the port closed cleanly. On the Pi (a Pi 5, the appliance host) with the development peer on `/dev/ttyACM0`: every display state driven from the peer; twenty three cable pulls and reinsertions, each recovered by both sides with no repair step, plus a restart of the peer; 26 presses received and none refused, with `LEFT_SHORT` from the gallery page and `RIGHT_SHORT` from the Wi-Fi page; `dmesg` and `udevadm` recorded the identity (`2e8a:0009`, `StopBath Pico Remote`, serial `525541546A1CAF09`, interface `00`), and the serial string was the same after a reboot of the Pi. Observed alongside: twice the peer hit "Permission denied" on the fresh node for the moments before udev applied the group, then succeeded; a Pi side race, not the remote's. Not measured: USB draw beside the radio adapter (see below). |

| `KE5` | scanning succeeds from a representative set of phones at arm's length in daylight; the phones, the module size, the distance and the lighting recorded | CLEARED 2026-09-15 by the author: "both codes scanned fine from arm's length on a few phones", with the layout demo's fixture 5 (the 53 byte Wi-Fi code, version 3 at 7 pixels a module, about 43 mm across) and fixture 6 (the gallery address, version 1 at 8 pixels, about 36 mm). Not recorded: the handsets and their operating system versions, the distance at which each first scanned, and the lighting; the 4 pixel minimum module size stands as the Appendix A value until a phone fails on it. |

| `KE6` | a full session presented, started and ended from the Pico against the real appliance, then the same session from the dashboard with the Pico attached, with no difference in outcome; termination origin `peripheral` shown on the dashboard; the first Part 8 answer in `FIELD_NOTES.md` | CLEARED 2026-09-15 by the author, against the real appliance with its `pico` device profile, `/dev/stopbath/remote` and the existing `[peripheral]` table: the appliance connected `stopbath-pico` version 1; sessions started by KEY0 and ended by a held KEY0, with `peripheral` as the origin on the dashboard; a guest phone (SM-F926B, Android 15) joined from the Wi-Fi code on the panel and opened the gallery from the panel's address; four photographs raised the count by partial refresh with the code still; `GUEST_CONNECTED` arrived as a partial refresh; a mid session cable pull and reinsertion recovered with the full record resent; a session started from the dashboard reached the panel on its own and one was ended from the dashboard. No `GUARD`, `MALFORMED` or refusal in the journal across five sessions. The photographer's verdict is in `FIELD_NOTES.md`. |

## Measurements owed

| Measurement | Owed to | Status |
|---|---|---|
| which driver variant lights the panel | evaluation log 4.3 | observed 2026-09-15: V2 |
| that the keys are on GP15 and GP17 and read low when pressed | evaluation log 4.3 | observed 2026-09-15: both keys report short and long |
| full refresh time | evaluation log 4.3, `KE2` | measured 2026-09-15: about 1580 ms |
| partial refresh time and residue | evaluation log 4.3, `KE2` | measured 2026-09-15: about 480 ms, no residue at 24 |
| firmware size against the 4 MB flash | Flipper V1 report | recorded per build in the reproduction report |
| USB identity as Linux sees it | evaluation log 4.2, `KE4` | observed 2026-09-15: `2e8a:0009`, `StopBath Pico Remote`, serial `525541546A1CAF09` stable across a reboot, interface `00` |
| total draw beside the appliance's radio adapter | evaluation log 4.5, `KE4` | unmeasured; the KE4 gate was cleared without it, and it stays owed (extension 5.3) |
