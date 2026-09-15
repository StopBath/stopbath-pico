# Actual contract evaluation

The Plan stage evidence log required by `STOPBATH_PICO_SPEC.md` Part 4 and by
Flipper 0.2. Every external fact this project builds on is recorded here with
its source, its version and the date it was read. Anything not recorded here is
not known, whatever a search result, a forum or an earlier message said.

Sections follow the specification's Part 4. A section that has not been
worked says so. Nothing in this file claims a hardware gate has passed.

## Reading this log

Each entry gives: source, version or commit, retrieval date, what was observed
(quoted where it matters), what remains uncertain, what experiment settles it,
and what was concluded. An entry marked OBSERVED was seen on the device by the
author; everything else is documentation.

---

## 4.1 SDK and toolchain

### pico-sdk release tags

- Source: https://github.com/raspberrypi/pico-sdk/releases
- Retrieved: 2026-09-15
- Observed: `2.3.1` is the newest release tag listed. The page describes it as
  a minor release with bug fixes and documentation improvements.
- Uncertain: the release dates the page summariser returned were internally
  inconsistent and are not recorded. The commit hash of the tag is recorded
  when the SDK is cloned in `KE1`.
- Conclusion: pin `2.3.1` as the starting point (Appendix A). Confirm on clone.

### Pico 2 W board header

- Source: https://raw.githubusercontent.com/raspberrypi/pico-sdk/2.3.1/src/boards/include/boards/pico2_w.h
- Retrieved: 2026-09-15
- Observed, verbatim from the header:
  - `#define RASPBERRYPI_PICO2_W`, `#define PICO_RP2350A 1`
  - `#define PICO_DEFAULT_SPI 0`, SCK 18, TX 19, RX 16, CSN 17
  - no `PICO_DEFAULT_LED_PIN`; `#define CYW43_WL_GPIO_LED_PIN 0`
  - `#define PICO_FLASH_SIZE_BYTES (4 * 1024 * 1024)`
  - CYW43 pins: `WL_REG_ON 23u`, `WL_DATA_OUT 24u`, `WL_DATA_IN 24u`,
    `WL_HOST_WAKE 24u`, `WL_CLOCK 29u`, `WL_CS 25u`
- Conclusion: the board name is `pico2_w`. GPIO 23, 24, 25 and 29 are taken by
  the radio and must not be used for the panel or the keys. None of the panel's
  documented pins (GP8 to GP13) nor either candidate key pair (GP2 and GP3, or
  GP15 and GP17) collides with them. The panel's SPI pins (CLK GP10, DIN GP11)
  are SPI1 function pins on the RP2350, not the board's default SPI0:
  `src/rp2350/hardware_regs/include/hardware/regs/io_bank0.h` defines
  `IO_BANK0_GPIO10_CTRL_FUNCSEL_VALUE_SPI1_SCLK` and
  `IO_BANK0_GPIO11_CTRL_FUNCSEL_VALUE_SPI1_TX` (read 2026-09-15), so
  `firmware/DEV_Config.c` uses `spi1` and `GPIO_FUNC_SPI` on those two pins,
  as Waveshare's own configuration does. The SDK signatures the firmware
  calls were read from `hardware/gpio.h`, `hardware/spi.h`, `pico/time.h`
  and `hardware/timer.h` at the pinned tag before use.

### `stdio_usb` line ending behaviour

- Source: `src/rp2_common/pico_stdio/include/pico/stdio.h` in the author's
  checkout at tag `2.3.1`, commit `079c6f39`.
- Retrieved: 2026-09-15
- Observed, verbatim, lines 24 to 31:

```c
// PICO_CONFIG: PICO_STDIO_ENABLE_CRLF_SUPPORT, Enable/disable CR/LF output conversion support, type=bool, default=1, group=pico_stdio
#ifndef PICO_STDIO_ENABLE_CRLF_SUPPORT
#define PICO_STDIO_ENABLE_CRLF_SUPPORT 1
// PICO_CONFIG: PICO_STDIO_DEFAULT_CRLF, Default for CR/LF conversion enabled on all stdio outputs, type=bool, default=1, depends=PICO_STDIO_ENABLE_CRLF_SUPPORT, group=pico_stdio
#ifndef PICO_STDIO_DEFAULT_CRLF
#define PICO_STDIO_DEFAULT_CRLF 1
```

- Conclusion: confirmed. By default every stdio output rewrites a line feed
  as carriage return plus line feed, which the protocol refuses as malformed.
  The firmware enables no stdio at all (`firmware/CMakeLists.txt` passes 0 to
  both `pico_enable_stdio_usb` and `pico_enable_stdio_uart`), and `KE4` drives
  TinyUSB's CDC calls directly. Waveshare's `DEV_Config.c`, which calls
  `stdio_init_all`, is not vendored for this reason (`lib/waveshare/PROVENANCE.md`).

### TinyUSB configuration

- Source: `src/rp2_common/pico_stdio_usb/include/tusb_config.h` at the
  pinned tag, and `lib/tinyusb/hw/bsp/rp2040/family.cmake` lines 68 and 69.
- Retrieved: 2026-09-15
- Observed: the SDK's own configuration sets `CFG_TUSB_RHPORT0_MODE`
  `OPT_MODE_DEVICE`, `CFG_TUD_CDC` 1, and the CDC RX, TX and endpoint buffer
  sizes (64 at full speed); `CFG_TUSB_MCU` and `CFG_TUSB_OS` are supplied by
  the SDK's TinyUSB build, not by the project's `tusb_config.h`. The SDK's
  `tinyusb_device` target is what a project links; `pico_stdio_usb` links the
  unmarked variant of the same.
- Conclusion: `firmware/tusb_config.h` follows the SDK's, with the RX and TX
  FIFOs raised to 512 so a whole protocol line fits between two services of
  the loop. The remote links `tinyusb_device`, `tinyusb_board` and
  `pico_unique_id`.

### Toolchain

- Source: https://raw.githubusercontent.com/raspberrypi/pico-sdk/2.3.1/README.md
- Retrieved: 2026-09-15
- Observed: the SDK asks for "CMake (at least version 3.13), python 3, a
  native compiler, and a GCC cross compiler" and names no GCC version. Its
  top level `CMakeLists.txt` says `cmake_minimum_required(VERSION 3.13...3.27)`.
  `cmake/pico_pre_load_toolchain.cmake` reads `PICO_TOOLCHAIN_PATH` from the
  environment and globs `${PICO_TOOLCHAIN_PATH}/bin/*gcc*`, which is how
  `scripts/build_firmware.sh` points the build at the downloaded compiler.
- Arm GNU Toolchain 15.2.rel1: the three archives below answered HTTP 200 on
  2026-09-15 and Arm's published `.sha256asc` digests were fetched beside
  them; both are pinned in `scripts/toolchain_versions.env`.
  - `arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz`, sha256
    `597893282ac8c6ab1a4073977f2362990184599643b4c5ee34870a8215783a16`
  - `arm-gnu-toolchain-15.2.rel1-mingw-w64-x86_64-arm-none-eabi.zip`, sha256
    `7936cac895611023ffb22a64b8e426098c7104cb689778c1894572ca840b9ece`
  - `arm-gnu-toolchain-15.2.rel1-mingw-w64-i686-arm-none-eabi.zip` also
    exists; not pinned, the host is x86_64.
  Scoop's `extras` bucket manifest for `gcc-arm-none-eabi` names the same
  `15.2.rel1` (retrieved 2026-09-15) but was not used: a manifest moves, a
  pinned URL and digest do not.
- The author's machine on 2026-09-15: host `gcc` 15.2.0 (scoop), `make`
  4.4.1, cmake 4.4.3, ninja 1.13.2, `unzip`, `tar`, `xz`, `sha256sum`,
  `curl`; no `arm-none-eabi-gcc` until `scripts/setup_toolchain.sh` is run.
  pico-sdk cloned by the author to `../pico-sdk` at tag `2.3.1`, commit
  `079c6f39023649b154152db30f1d781e884879bc`, with submodules btstack
  `v1.7-rc1-1219`, cyw43-driver `v1.1.1`, lwip `STABLE-2_2_1_RELEASE`,
  mbedtls `v3.6.0-2128`, tinyusb `0.18.0`.

---

## 4.2 USB CDC

### Default USB descriptors

- Source: https://raw.githubusercontent.com/raspberrypi/pico-sdk/master/src/rp2_common/pico_stdio_usb/stdio_usb_descriptors.c
- Retrieved: 2026-09-15, from `master`; to be re-read at the pinned tag in `KE4`.
- Observed, verbatim:

```c
#ifndef USBD_VID
#define USBD_VID (0x2E8A) // Raspberry Pi
#endif

#ifndef USBD_PID
#if PICO_RP2040
#define USBD_PID (0x000a) // Raspberry Pi Pico SDK CDC for RP2040
#else
#define USBD_PID (0x0009) // Raspberry Pi Pico SDK CDC
#endif
#endif
```

```c
#define USBD_ITF_CDC       (0) // needs 2 interfaces
#if !PICO_ENABLE_USB_RESET_VIA_VENDOR_INTERFACE
#define USBD_ITF_MAX       (2)
#else
#define USBD_ITF_RPI_RESET (2)
#define USBD_ITF_MAX       (3)
#endif
```

  The string descriptors are manufacturer `Raspberry Pi`, product `Pico`,
  and a serial string filled by `pico_get_unique_board_id_string`. `bDeviceClass`
  is `TUSB_CLASS_MISC` with the IAD protocol. One CDC function, occupying
  interfaces 0 (control) and 1 (data).
- Read again at the pinned tag on 2026-09-15 in the author's checkout: the
  same values, plus the configuration descriptor (`TUD_CONFIG_DESCRIPTOR`
  with bus powered attributes and 250 mA, `TUD_CDC_DESCRIPTOR` on interface 0
  with endpoints `0x81`, `0x02`, `0x82`, packet sizes 8 and 64) and the
  string callback, which the SDK caps at `USBD_DESC_STR_MAX` 20 units.
- Decided in `KE4` (`firmware/usb_descriptors.c`): the same descriptors, with
  vendor `2E8A` and product `0009` kept, the product string
  `StopBath Pico Remote`, the interface string `StopBath peripheral link`,
  the serial string the flash unique id, and the string cap raised to 32 so
  the product name is not cut. Raspberry Pi's `usb-pid` repository (read
  2026-09-15) lists `0x0009` as "Raspberry Pi Pico SDK CDC UART" and says a
  standard interface device needs no separate product id and can be
  identified by its product string, which is what is done. Whether the
  unique board id serial string is identical across reboots and
  re-enumerations is believed but unobserved. What Linux `cdc_acm` shows as
  `bInterfaceNumber` for the tty is unobserved.
- Experiment, `KE4` gate: `lsusb -v` and `udevadm info -a` on the appliance
  with the Pico attached, twice, across a reboot of each side.
- Conclusion so far: a Pico 2 W presents vendor `2E8A`, product `0009`, one
  CDC channel, and therefore one `ttyACM` node. This is what
  the StopBath repository's `docs/PICO_REMOTE_HANDOFF.md` asks the appliance's device rule to match.

### DTR and suspend

- Source: TinyUSB `0.18.0` as vendored at `lib/tinyusb` in the pinned SDK:
  `src/class/cdc/cdc_device.h` and `src/device/usbd.h`.
- Retrieved: 2026-09-15
- Observed: the declarations exist with these signatures:
  `bool tud_cdc_n_connected(uint8_t itf);`,
  `TU_ATTR_WEAK void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts);`,
  `void tud_mount_cb(void);`, `void tud_umount_cb(void);`,
  `void tud_suspend_cb(bool remote_wakeup_en);`, `void tud_resume_cb(void);`.
- Read in `KE4`, 2026-09-15, same sources:
  - `tud_cdc_n_connected` (cdc_device.c line 132) is
    `tud_ready() && tu_bit_test(_cdcd_itf[itf].line_state, 0)`, and
    `tud_ready` (usbd.h line 97) is `tud_mounted() && !tud_suspended()`.
    So the one call is the Flipper's whole table: cable present (mounted,
    not suspended) and DTR asserted.
  - `line_state` is set from `CDC_REQUEST_SET_CONTROL_LINE_STATE` (line
    403 on), bit 0 DTR, bit 1 RTS, and `cdcd_reset` (line 290) clears the
    interface structure up to `wanted_char`, which includes `line_state`.
    A bus reset therefore clears DTR, and the stale DTR the Flipper had to
    revalidate after a re-enumeration cannot occur here.
  - The stack is serviced by `tud_task()` from the loop; `tusb_init()` is
    the macro form of `tusb_rhport_init(0, NULL)` and returns a bool, which
    `firmware/usb_link.c` checks.
- Conclusion: `firmware/usb_link.c` polls `tud_ready()` and
  `tud_cdc_n_connected(0)` every tick and passes them to the decision table
  in `transport/remote_link_edge.c`, which is tested on the host against the
  Flipper's table. No callback is needed. When each fact changes across a
  physical cable pull, and how quickly, is observed at the `KE4` gate. The Flipper's transport (`remote_transport.c`) found
  that a physical cable pull produces no DTR drop and that a resume can arrive
  with no suspend before it, leaving cached DTR stale; the Pico transport must
  be checked for the same two cases.

---

## 4.3 Display

### Module

- Source: https://www.waveshare.com/wiki/Pico-ePaper-4.2
- Retrieved: 2026-09-15 (the page refuses non-browser fetches; read in a
  browser).
- Observed, quoted: "4.2inch e-Paper display module for Raspberry Pi Pico, 400
  x 300 pixels, support black, white, and 4 grayscale display, SPI interface";
  "2 x user buttons for easy interacting"; "Full refresh: 4s"; "Refreshing
  power: 26.4mW(typ.)"; "The global refresh process will have a flickering
  effect, this is a normal phenomenon". Pinout table for the 8 pin cable:

| e-Paper | Pico | Description |
|---|---|---|
| VCC | VSYS | Power input |
| GND | GND | Ground |
| DIN | GP11 | MOSI |
| CLK | GP10 | SCK |
| CS | GP9 | Chip select, low active |
| DC | GP8 | Data/Command, high is data |
| RST | GP12 | Reset, low active |
| BUSY | GP13 | Busy output |

  Resources section links a "UC8176 (controller) datasheet". Demo code at
  https://github.com/waveshare/Pico_ePaper_Code (C and MicroPython). The FAQ
  says a full refresh should follow "5 rounds of partial refreshing" to clear
  residue, and recommends sleep or power off between refreshes.
- Uncertain: the page does not state the key GPIOs and does not say which of
  the two 4.2 inch drivers in the demo repository matches the board sold
  today. The user's board is marked, as reported 2026-09-15: `kc r r p2r
  pico2w`, which does not resolve the variant. The wiki's mention of a "Pixel &
  Byte" section for the 3.7 inch panel is copied text and not evidence about
  this one.

### Retail listing

- Source: https://thepihut.com/products/4-2-e-paper-display-module-for-raspberry-pi-pico-black-white-400x300
- Retrieved: 2026-09-15
- Observed: "400 x 300 pixels", "Full refresh time: 4s", "Greyscale: 4",
  "2x user buttons", and partial refresh listed as "N/A".
- Conclusion: the listing's "N/A" conflicts with the driver headers below,
  which both expose a partial refresh function. Treated as unknown until
  measured on the panel in `KE2`.

### Demo driver headers

- Source: https://github.com/waveshare/Pico_ePaper_Code/tree/main/c/lib/e-Paper
- Retrieved: 2026-09-15, from `main`; the commit is recorded when vendored.
- Observed: the directory holds `EPD_4in2`, `EPD_4in2_V2`, `EPD_4in2b_V2` and
  `EPD_4in2b_V2_old`. The `b` variants are three colour and not this panel.

`EPD_4in2_V2.h`, https://raw.githubusercontent.com/waveshare/Pico_ePaper_Code/main/c/lib/e-Paper/EPD_4in2_V2.h:

```c
#define EPD_4IN2_V2_WIDTH       400
#define EPD_4IN2_V2_HEIGHT      300
#define Seconds_1_5S      0
#define Seconds_1S        1
#define KEY0      15
#define KEY1      17
void EPD_4IN2_V2_Init(void);
void EPD_4IN2_V2_Init_Fast(UBYTE Mode);
void EPD_4IN2_V2_Init_4Gray(void);
void EPD_4IN2_V2_Clear(void);
void EPD_4IN2_V2_Display(UBYTE *Image);
void EPD_4IN2_V2_Display_Fast(UBYTE *Image);
void EPD_4IN2_V2_Display_4Gray(UBYTE *Image);
void EPD_4IN2_V2_PartialDisplay(UBYTE *Image, UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend);
void EPD_4IN2_V2_Sleep(void);
```

`EPD_4in2.h`, https://raw.githubusercontent.com/waveshare/Pico_ePaper_Code/main/c/lib/e-Paper/EPD_4in2.h:

```c
#define EPD_4IN2_WIDTH       400
#define EPD_4IN2_HEIGHT      300
void EPD_4IN2_Init_Fast(void);
void EPD_4IN2_Init_Partial(void);
void EPD_4IN2_Init_4Gray(void);
void EPD_4IN2_Clear(void);
void EPD_4IN2_Display(UBYTE *Image);
void EPD_4IN2_PartialDisplay(UWORD X_start, UWORD Y_start, UWORD X_end, UWORD Y_end, UBYTE *Image);
void EPD_4IN2_4GrayDisplay(const UBYTE *Image);
void EPD_4IN2_Sleep(void);
```

- The demo repository was cloned on 2026-09-15 at commit
  `c9bcd84db5adf5f085353649a8a5c31492bc5fb8` (dated 2024-10-24). Its
  `c/lib/Config/DEV_Config.c` selects `spi1` at 4 MHz and the six pins above,
  calls `stdio_init_all` in `DEV_Module_Init`, and passes `GPIO_OUT` where a
  `gpio_function_t` is expected; the drivers themselves call only
  `DEV_Digital_Write`, `DEV_Digital_Read`, `DEV_SPI_WriteByte`, `DEV_Delay_ms`
  and the `Debug` macro. The two 4.2 inch drivers were vendored verbatim
  (`lib/waveshare/PROVENANCE.md`); the configuration layer was written fresh.
- Uncertain: which driver the board in hand needs. The V1 driver's only full
  mode initialiser is `EPD_4IN2_Init_Fast` (its comment names an `EPD_4IN2_Init`
  that does not exist). The V2 driver speaks a different command set
  (`0x12` soft reset, `0x24` and `0x26` RAM, `0x22` and `0x20` update).
- Experiment, `KE1` gate: two first light images were built, one per driver;
  the author flashed V1 first and V2 when the panel stayed blank.
- OBSERVED by the author, 2026-09-15: the `EPD_4in2_V2` image drew; the
  `EPD_4in2` image did not ("v1 didn't work v2 did"). The panel in hand
  therefore speaks the V2 command set, whatever the wiki's UC8176 datasheet
  link suggests. The V1 driver and image were removed the same evening
  (`lib/waveshare/PROVENANCE.md`).
- OBSERVED by the author, 2026-09-15: "Key0 and Key1 both update on short
  and long". The schematic's GP15 and GP17, active low with internal pull
  ups, are confirmed, and the input model's classification holds on the
  device at the Appendix A thresholds (1000 ms, 20 ms debounce), which stay
  as starting points until `KD11`.
- OBSERVED by the author, 2026-09-15: "if you press while the screen is
  refreshing it doesn't record the new value". Expected: the first light
  program calls the driver's blocking full refresh and samples no key until
  it returns. It sets the requirement for `KE2` that key sampling and the
  panel's busy wait be decoupled (the driver spins on the BUSY line with a
  10 ms sleep per iteration, `EPD_4IN2_V2_ReadBusy`), and it is the first
  data point on how long a full refresh actually takes: long enough to lose a
  press, which a photographer will do.
- OBSERVED by the author, 2026-09-15, about the cable: two micro USB cables
  produced no enumeration at all on the PC in BOOTSEL mode (no `RP2350`
  drive, no device of any kind); a third cable worked at once. Recorded so
  the next person who sees "nothing happens" tries the cable before the
  board. A data capable micro USB cable is a hardware requirement.

### Partial refresh, from the driver source

- Source: `lib/waveshare/EPD_4in2_V2.c` at the vendored commit, lines 501
  to 557 (`EPD_4IN2_V2_PartialDisplay`) and 144 to 149
  (`EPD_4IN2_V2_TurnOnDisplay_Partial`), read 2026-09-15.
- Observed: the partial sequence writes border waveform `0x3C 0x80` and
  update control `0x21 0x00 0x00`, sets the RAM window (`0x44`, `0x45`) and
  cursor (`0x4E`, `0x4F`) in byte columns and pixel rows, streams the
  rectangle into RAM plane `0x24` only, then updates with `0x22 0xFF`,
  `0x20`. The full sequence (`Display`, lines 356 to 380) streams the whole
  frame into both `0x24` and `0x26` and updates with `0x22 0xF7`. `Init`
  (lines 225 to 250) sets `0x21 0x40 0x00` and `0x3C 0x05`, which the
  partial pass overwrites. A fast full mode exists (`Init_Fast`, `0x1A`
  temperature override, `0x22 0xC7`), unmeasured and unused.
- Conclusion: regions must start on a byte column and span whole bytes,
  which the layout enforces and a test holds. `firmware/panel.c` restores
  the two registers before a full refresh rather than resetting.
- OBSERVED by the author, 2026-09-15, with the KE2 layout demo: partial
  refreshes of the count region and of the error band took on the glass;
  full refreshes after partials drew correctly (so restoring the two
  registers without a reset suffices); "no residue or ghosting old digits"
  after four consecutive partials; every fixture legible ("everything else
  looks good"); the forced full refresh on the fifth partial was visible as
  a black flash and was at first mistaken for a fault, which is the product
  argument for a high bound. The bound was raised from 5 to 25 as the next
  measurement point.
- OBSERVED by the author, 2026-09-15, durations from the diagnostic screen
  read after one refresh of each kind: a partial refresh of the count
  region (136 by 80 pixels) about 480 ms; a full refresh about 1580 ms.
  The manufacturer's "Full refresh: 4s" is therefore not this panel's
  figure (it may describe the UC8176 variant). Both include the SPI transfer
  at 4 MHz with a chip select toggle per byte, so the panel's own update
  time is somewhat less; a faster transfer is possible if it ever matters.
- OBSERVED by the author, 2026-09-15: "no ghosting or residual at all" after
  24 consecutive partial refreshes of the count region. The forced full
  bound was moved to 100 as the next measurement point; the manufacturer's
  five is discarded for this panel.

### Schematic

- Source: https://files.waveshare.com/upload/2/20/Pico-ePaper-4.2.pdf, linked
  from the wiki's Resources section as "Schematic".
- Retrieved: 2026-09-15, sha256
  `86d78a59901b965a43c736fd464653d4aaf9e972f24485d0e0babcd10ee18c10`.
- Observed, sheet 1: the "Key" block shows `KEY0` through switch `K1` to
  `GND` and `KEY1` through switch `K2` to `GND`, with no pull up resistor on
  either net. The "Connector_Pico" block shows net `KEY0` on Pico pin 20
  (`GP15`) and net `KEY1` on Pico pin 22 (`GP17`). The panel nets are on
  `GP8` (DC), `GP9` (CS), `GP10` (SCK), `GP11` (DIN), `GP12` (RST), `GP13`
  (BUSY), through a TXS0108E level shifter. Power is `VSYS` by default via
  jumper `J2`. Jumper `J3` selects 3 line or 4 line SPI, default 4 line
  (`BS` to ground). Sheet 2 is the board outline with `KEY0` and `KEY1`
  labelled beside `K1` and `K2` on the right edge.
- Conclusion: the keys are `GP15` and `GP17`, active low, needing the RP2350's
  internal pull ups, which `firmware/first_light.c` enables. The V2 driver
  header's `KEY0 15` and `KEY1 17` agree; the GP2 and GP3 figure from a search
  summary was wrong and is discarded. Confirmed on the device at the `KE1`
  gate.
- Conclusion so far: both candidate drivers expose partial refresh and a
  15000 byte one bit frame buffer (400 times 300 over 8). Frame buffer size is
  no concern on an RP2350 with 520 KB of SRAM, but the number is recorded so
  the buffer is sized once at start (Flipper 0.10).

---

## 4.4 QR

Not yet worked. The Flipper repository's published vectors
(`tests/qr_published_vectors.h`) and its vendored `lib/qrcodegen` at its recorded
commit and digest are the starting point; they are copied with provenance in
`KE5`. Encode and render time and scan reliability are measured on the device.

---

## 4.5 Power

Not yet measured. The panel's quoted refreshing power is 26.4 mW (wiki, above).
The Pico 2 W's draw with the radio off is not recorded anywhere this project
has read. Measured at the `KE4` gate beside the appliance's radio adapter.

---

## Part 6 of the extension: how the device appears to the appliance

Not yet observed. What the appliance needs to know (extension Part 6, and its
`scripts/hardware/47-peripheral-device.sh`): how the device appears when
attached, whether its identifier is stable across reattachment and reboot, how
attach and detach are detected, and what happens to an open handle when the
device disappears. The Flipper's answers are in the StopBath repository's
evaluation log; the Pico's are recorded here at the `KE4` gate and then carried
into the StopBath repository's `docs/PICO_REMOTE_HANDOFF.md`.

---

## Decisions still needing the author

See `STOPBATH_PICO_SPEC.md` Part 10. Open at 2026-09-15: `KD8` (layouts, at
the `KE2` gate), `KD9` (Key1 alignment, after `KE6`), `KD10` (wireless, in the
StopBath repository), `KD11` (press thresholds, after `KE1`).
