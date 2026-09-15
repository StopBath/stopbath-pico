# StopBath Pico Remote

A specification for handoff to a coding agent.

A dedicated physical control surface for a StopBath appliance, built on a
Raspberry Pi Pico 2 W and a Waveshare 4.2 inch e-paper module, so the
photographer can present, start and terminate a session without taking a phone
out, and so a guest can scan a large, daylight readable code.

This is the second implementation of the StopBath peripheral protocol. The
first is the Flipper Zero application in `stopbath-flipper`, whose specification
(`STOPBATH_FLIPPER_SPEC.md`) anticipated exactly this device in its Part 9: "a
microcontroller with an e-ink display". This document is a companion to
`STOPBATH_SPEC.md` and `docs/PERIPHERAL_EXTENSION.md` in the StopBath
repository and to `STOPBATH_FLIPPER_SPEC.md`. It replaces no rule in any of
them. Where this document and any of those disagree, the other wins and the
disagreement is a defect to raise.

## How to read this document

Linear. Read it top to bottom once, then work the stages in order.

Every normative line carries a tag. An untagged sentence is context, not a
requirement.

| Tag | Meaning |
|---|---|
| **MUST** | Mandatory. Not negotiable. Failing it fails the phase. |
| **MUST NOT** | Prohibited. Not negotiable. |
| **DECIDE** | A blocking decision belonging to the author. Numbered `KD1` and up. Stop and ask. Never guess. |
| **GUIDANCE** | A suggestion with reasoning. Depart from it only by saying so and why. |

Identifiers follow the scheme shared by the other documents. The second letter
says what kind of thing it is and the first says which document owns it.

| Prefix | Meaning |
|---|---|
| `E1` and up, `D1` and up | phases and decisions in `STOPBATH_SPEC.md` |
| `PE1` and up, `PD1` and up | phases and decisions in `docs/PERIPHERAL_EXTENSION.md` |
| `FE1` and up, `FD1` and up | phases and decisions in `STOPBATH_FLIPPER_SPEC.md` |
| `KE1` and up | an execute phase in this document |
| `KD1` and up | a decision in this document |

`K` was chosen (`KD2`, settled 2026-09-15) because it is a prefix of no existing
decision prefix and reads as nothing else.

Concrete values are illustrative unless they appear in **Appendix A**, which
lists values that are unverified and must be settled during the Plan stage.

### What this document is not

It was written having read the Waveshare wiki page for the module, two of
Waveshare's demo driver headers, and the pico-sdk's USB descriptor source and
Pico 2 W board header, all retrieved on 2026-09-15 and recorded in
`docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md`. Nothing else about the Pico,
the SDK, TinyUSB, the display controller or the board's key wiring has been
verified. Every such claim is a question for the Plan stage, never a fact to
build on.

---

# Part 0: Absolute rules

**Part 0 of `STOPBATH_FLIPPER_SPEC.md` applies in full and is not restated
here.** That is: the git prohibition (0.1), evidence before code (0.2), blocked
means ask in the five part form batched by stage (0.3), tests first (0.4),
naming (0.5), comments explain why (0.6), do not repeat yourself (0.7), no em
dashes and no converter mangled hyphen runs (0.8), no placeholders (0.9), the C
rules (0.10), contracts sacred once accepted (0.11), dependencies (0.12) and the
consolidated prohibitions (0.13). Restating them would breach 0.7.

Where the Flipper text says "Flipper", "furi", "FAP" or "SDK", read "Pico",
"pico-sdk", "firmware image" and "pico-sdk" respectively. Where it says
"a Flipper API", read "a pico-sdk, TinyUSB or Waveshare driver function".

Two additions, because the hardware differs:

## 0.14 Two languages, one rule set

The firmware is C on pico-sdk (`KD1`). Host tests, the typography scan and the
SDK setup script are the only places a second language appears (Python for the
scans and generators, exactly as in the Flipper repository; shell for setup).
**MUST NOT** add a third.

## 0.15 The display is guest facing

The Flipper's screen faced the photographer. This one faces the guest, who is a
stranger standing at arm's length in daylight. **MUST** treat scan reliability
by a stranger's phone as the primary display requirement, ahead of any status
text, and **MUST** verify it by scanning, never by arithmetic (Flipper 2.7).

---

# Part 1: Intent and scope

## 1.1 What it is

The device is a StopBath peripheral in the sense of `docs/PERIPHERAL_EXTENSION.md`
and speaks protocol version 1 as frozen in `docs/peripheral/PROTOCOL.md` and
`docs/peripheral/protocol.json` in the StopBath repository. It is not StopBath.
It reports what physically happened and renders what it is sent. StopBath
remains authoritative for every session, every guest and the meaning of every
button (Flipper 2.1, the single most important rule, applies unchanged).

The first implementation attaches to the appliance by USB and is a proving
build for the interaction model on this hardware. A later phase may carry the
same protocol over the appliance's trusted Wi-Fi network (Part 9).

## 1.2 Goals

The first useful version:

- attaches to the Pi by USB and appears as one serial device
- completes the protocol handshake and renders every display state it is sent
- renders the Wi-Fi join code and the guest gallery code at a size a stranger
  can scan from arm's length
- starts a session on a short press of Key0 and requests termination on a long
  press of Key0
- switches between the two code screens on a press of Key1
- shows a clear not connected indication when the link is down
- reconnects without a repair step when either side restarts
- keeps all meaningful logic and all session state in StopBath

## 1.3 Non-goals

Everything in Flipper 1.3 applies. In addition, for the first version:

**MUST NOT** use the Pico 2 W's radio for anything. Wireless transport is Part 9
and is blocked on appliance side work that has not been agreed.

**MUST NOT** present NFC. The Pico has no NFC hardware; the QR surface alone is
a working product (Flipper 2.7, FE6 precondition).

**MUST NOT** implement a screen lock (`KD5`). See 2.4.

**MUST NOT** become required. A StopBath appliance with no Pico attached
**MUST** remain fully operable through the dashboard, unchanged.

## 1.4 Radical ownership

Flipper 1.4 applies. This repository is the evidence that it works: the
protocol was implemented on different hardware without the appliance changing
its Go code.

---

# Part 2: The peripheral contract

## 2.1 Ownership

Flipper 2.1 and 2.2 apply verbatim, with "Pico" for "Flipper". The one place
this device knows more than the Flipper did is the Key1 mapping in 2.3, which
is a recorded deviation, bounded, and never destructive.

## 2.2 Controls

The module has two user buttons, `KEY0` and `KEY1`. There is no other input.

| Input | Handled | Event reported |
|---|---|---|
| Key0 short press | by StopBath | `CENTER_SHORT` |
| Key0 long press | by StopBath | `CENTER_LONG` |
| Key1 short press | by StopBath | `LEFT_SHORT` or `RIGHT_SHORT`, see 2.3 |
| Key1 long press | unused | none |

The current StopBath interpretation, which lives in StopBath and is a product
hypothesis rather than protocol semantics (extension 2.1, seam 3.5): a short
centre press while idle starts a session; a long centre press while active
requests termination; `LEFT_SHORT` shows the `WIFI` page; `RIGHT_SHORT` shows
the `GUEST` page. In the photographer's words (2026-09-15): Key0 is starting
and killing the Wi-Fi, Key1 goes between the two QR screens.

**MUST NOT** report anything for a Key1 long press in the first version. It is
reserved, so that a lock (2.4) or a second page action can be added without
moving anything that already reports.

**MUST** classify a press as short or long on the device from a measured hold
time, since the Pico has no firmware input layer to do it. The threshold is
Appendix A.

## 2.3 The Key1 mapping (`KD4`)

The appliance's page events are absolute: `LEFT_SHORT` means the `WIFI` page
and `RIGHT_SHORT` means the `GUEST` page (seam 3.5). One key cannot cycle
between them without the device choosing which to send.

Settled for the proving phase, 2026-09-15: the device sends `RIGHT_SHORT` when
the last `DISPLAY` record it received named page `WIFI`, and `LEFT_SHORT` when
it named `GUEST`. When the last record named `NONE`, or there is no record, it
sends `RIGHT_SHORT`, which the appliance answers with an unchanged record
outside an active session (seam 3.5, the not active row).

This derives the choice from the last record received, which the device already
holds in order to render it (Flipper 2.5). It does not hold a belief about the
session. What it does encode is which of two page events is the other one,
which is knowledge of the appliance's interpretation table and therefore a
deviation from Flipper 2.1.

**MUST** record it in `IMPLEMENTATION_DEVIATIONS.md` with a covering test.

**MUST** keep the mapping in exactly one function so that a later alignment
with the appliance (a protocol addition, or a change to what Key1 does) is one
edit.

**MUST NOT** let this mechanism, or any future refinement of it, select a
destructive event. Key1 may only ever choose between `LEFT_SHORT` and
`RIGHT_SHORT`.

The author has accepted (2026-09-15) that this is provisional: once the
behaviour is proven on hardware against the real appliance, the two
repositories align on whatever the field shows is right. That alignment is a
decision (`KD9`), not an implementation detail.

## 2.4 Screen lock and the guard flags

The appliance's guard requires `foregrounded=1` and `unlocked=1` on every
`BUTTON` (seam 3.5, Q6 accepted). This device satisfies both honestly and
implements neither as state:

- `foregrounded` is always `1`. The firmware is single purpose; there is no
  other application, no background and no desktop. This is the same reasoning
  and the same recorded deviation as the Flipper's (its
  `IMPLEMENTATION_DEVIATIONS.md`, foreground guard).
- `unlocked` is always `1`, `HELLO` carries `locked=0`, and `STATE` is never
  sent (`KD5`, settled 2026-09-15). The lock exists for the pocket case
  (Flipper 2.4). A bare Pico carrying a 4.2 inch panel, cabled to the
  appliance, is not pocketed. When the device is enclosed or wireless this is
  revisited, and Key1 long press is reserved for it.

**MUST** record both in `IMPLEMENTATION_DEVIATIONS.md`, each with the test
that holds the flag at its value.

**MUST NOT** weaken the appliance's enforcement to accommodate this. The
appliance still rejects a `0` in either flag, which is what protects it from a
different or faulty peripheral.

## 2.5 State ownership

Flipper 2.5 applies verbatim. The device renders the last record received,
replaces it wholly, holds no belief about the session, and shows not connected
rather than stale content when the link drops.

The only local state is: the link state, the last record, the key hold timers,
and the diagnostic counters.

## 2.6 Display

400 by 300 pixels, black and white, with an optional four grey mode. Full
refresh takes about four seconds and flickers visibly; the demo drivers expose
a partial refresh whose behaviour on this panel is unmeasured (Appendix A).

The e-paper changes what "publish on every change" costs. The appliance sends
a full record on every change to any field, including each increment of the
delivered count (extension 3.2). A four second flicker per photograph would
make the code unscannable exactly when the product is meant to work. So:

**MUST** redraw only the region whose value changed, using partial refresh
where the panel supports it, and reserve a full refresh for a change to the
code itself (page or payload), a change of link screen, and a periodic ghost
clearing pass. The regions and the policy are settled in `KE2` by measurement.

Amended 2026-09-15 at the start of `KE2`: the draft listed "a status change"
among the full refresh triggers. `PRESENTING` becomes `GUEST_CONNECTED` as the
first guest joins, which is exactly when others are still scanning, so a
status change is a partial refresh of the header and hint like any other
within-state change. The rule below already required this; the list above
contradicted it and was wrong.

**MUST NOT** let the code region refresh, partially or fully, for a change to
any field other than `page` or `payload`. A guest part way through scanning
must not have the code move under them (extension 3.2, the last rule).

**MUST** treat the panel's own limits as constraints: the manufacturer advises
a full refresh after several partial refreshes to clear residue, and sleep or
power off between refreshes. Record what was observed.

Conceptual states, to be laid out against the real geometry in `KE2` (`KD8`):

```text
┌──────────────────────────────────┐   ┌──────────────────────────────────┐
│ STOPBATH            not connected│   │ STOPBATH                   READY │
│                                  │   │                                  │
│        Reconnect the cable       │   │     Key0: start a session        │
│                                  │   │                                  │
└──────────────────────────────────┘   └──────────────────────────────────┘

┌──────────────────────────────────┐   ┌──────────────────────────────────┐
│ STOPBATH   PRESENTING   WIFI 1/2 │   │ STOPBATH  GUEST_CONNECTED  2/2   │
│  ┌────────────────────────┐      │   │  ┌────────────────────────┐      │
│  │                        │ Join │   │  │                        │ Open │
│  │   large Wi-Fi code     │ this │   │  │   gallery address code │ the  │
│  │                        │ Wi-Fi│   │  │                        │ page │
│  └────────────────────────┘      │   │  └────────────────────────┘  3   │
│ Key1: gallery code    error band │   │ Key1: Wi-Fi code      delivered  │
└──────────────────────────────────┘   └──────────────────────────────────┘
```

**MUST** render the status code and the error code as text somewhere on every
record, never only as an icon, because the code set is what the appliance
promises and the dashboard's vocabulary is the same.

**MUST** show the delivered count. The photo count is the photographer's only
signal that a session has picked up an earlier group's photographs
(`STOPBATH_SPEC.md` 2.11).

## 2.7 Presentation payloads: QR only

Flipper 2.7 applies for the QR surface. The device generates and renders the
matrix locally from the payload the appliance sends. NFC is out (1.3).

At 400 by 300 the Wi-Fi payload has room. The appliance's real payload was
measured on the Flipper at exactly 53 bytes, the largest a version 3 code at
low error correction holds. Here a version 3 code at low error correction is 29
modules; at eight pixels a module with a four module quiet zone it is 296
pixels square, which fits the panel's short side. That arithmetic is sound and
its operational conclusion is not. **MUST** establish the module size, the
error correction level and the scan distance by physical testing with phones
that are not the author's (`KE5` hardware gate).

**MUST** refuse, with a distinct on screen error, a payload the encoder cannot
fit at the chosen version, rather than truncating it or rendering something
unscannable. The parser's bound (256 bytes) is not the display's; the two
failures stay distinguishable (Flipper `PROTOCOL.md`, facts from the hardware).

## 2.8 Protocol

The protocol is version 1 as frozen in the StopBath repository. This repository
carries a copy of `protocol.json` and a test that holds the copy equal to the
appliance's, as the Flipper repository does. Rule 0.11 applies in full: nothing
is renamed, aliased, normalised or improved.

**MUST NOT** propose a protocol change from this repository. A change the field
shows is needed (for instance a relative page event, see `KD9`) is raised in
the StopBath repository as a version 2 decision, because StopBath owns the
contract (extension 4.1).

## 2.9 Connection and recovery

Flipper 2.10 applies verbatim.

The transport is USB CDC (`KD1`). On the appliance side nothing changes: it
opens a device path, asserts DTR, and treats hangup as detach (StopBath
`internal/infrastructure/peripherallink/serial_linux.go`). On the Pico, DTR
asserted is the port opened event and DTR dropped or USB suspend is the port
closed event, exactly as the Flipper's transport does it. Whether pico-sdk's
CDC layer exposes DTR and suspend the way this needs is a Plan stage question
(Part 4).

## 2.10 Observability

Flipper 2.11 applies. The diagnostic view is a screen the photographer can
reach without a computer. **GUIDANCE** Hold Key1 while the link is down to show
it, since Key1 has no reported meaning then. Record the gesture in the README.

---

# Part 3: Architecture and repository boundary

Three repositories, one protocol:

```text
stopbath.photo        the appliance; owns the protocol definition
stopbath-flipper      the first peripheral
stopbath-pico         this one
```

Layout, mirroring the Flipper repository so a contributor moving between the
two meets one shape:

| Path | Contents |
|---|---|
| `firmware/` | the pico-sdk facing application, kept thin: main loop, GPIO, SPI, TinyUSB glue |
| `remote_input/` | pure logic: two keys, hold timing, short and long classification, no SDK |
| `remote_display/` | pure logic: layout for 400 by 300, fixtures, the QR wrapper, region change detection, no SDK |
| `protocol/` | the parser and encoder and the tables generated from `protocol.json`, copied from the Flipper repository with provenance |
| `session/` | the client session: handshake, replacement, the guard flags, the Key1 mapping, no SDK |
| `peer/` | the development peer, copied from the Flipper repository with provenance (see `KE3`) |
| `fuzz/` | the protocol parser fuzz harness |
| `lib/qrcodegen/` | the vendored QR encoder, unmodified, with its licence and provenance |
| `lib/waveshare/` | the vendored display driver for the panel variant in hand, unmodified, with provenance |
| `tests/` | host tests and the shared harness |
| `scripts/` | the typography scan, the SDK setup script, generators |
| `docs/evaluation/` | the Plan stage evidence log |
| `docs/` | `APPLIANCE_HANDOFF.md`, the changes asked of the StopBath repository |

**MUST** keep every module outside `firmware/` and `lib/` free of pico-sdk,
TinyUSB and driver includes, so it builds and tests on the development machine
with `gcc` and `make`, under the Flipper repository's warning set and its
sanitiser and allocation wrapping approach.

**MUST** compile the vendored display driver on the device only. Its GPIO and
SPI calls are the one edge the display module's pure logic hands a bitmap to.

**MUST** record the source repository, commit and file digest of everything
copied from the Flipper repository or from Waveshare, in a `PROVENANCE.md`
beside it. Copied code is the Flipper repository's under its MIT licence; the
attribution travels with it.

---

# Part 4: Stage one, Plan

**MUST NOT** write implementation code or a behavioural test until the Plan
stage has settled the phase being implemented.

`docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md` records, for every item below,
source URL, version or commit, retrieval date, relevant files, observed
contract, uncertainties, experiments performed, conclusions accepted and
decisions still needing the author. It already holds what was retrieved on
2026-09-15. Each phase's assumptions to verify name the section that must be
filled before that phase begins.

## 4.1 SDK and toolchain

Verify against the pinned pico-sdk tag: the board name for the Pico 2 W, the
CMake invocation, the toolchain version the SDK expects, the `.uf2` output,
and how the SDK's `stdio_usb` behaves with respect to line endings (it is
believed to translate a line feed to carriage return plus line feed by default,
which would make every message malformed on the wire; if so the transport
uses the TinyUSB CDC calls directly and never `stdio`). Record the exact
macros and their defaults.

## 4.2 USB CDC

Verify in pico-sdk and TinyUSB source: how the device learns DTR
(`tud_cdc_connected` or the line state callback), how it learns suspend and
resume, what the default descriptors are (vendor, product, serial string,
interface count), whether the serial string is stable across reboots, and what
`lsusb` and `udevadm info` show on the appliance's Debian release. The udev
rule in `docs/APPLIANCE_HANDOFF.md` depends on the last two.

## 4.3 Display

Verify against the board in hand: the driver variant (`EPD_4in2` or
`EPD_4in2_V2`), the controller, the GPIO the two keys are on (the evidence
conflicts: GP2 and GP3 from one source, GP15 and GP17 from the V2 driver
header), whether the keys are active low with pull ups, the SPI instance and
pins, full refresh time, partial refresh availability and its residue
behaviour, and the frame buffer size (15000 bytes for one bit per pixel).

## 4.4 QR

Measure on the device: encode time at versions 2 to 4, render time, and scan
reliability from a representative set of phones (`FD11` answered the set for
the Flipper; reuse it) at arm's length in daylight.

## 4.5 Power

The Pico and the panel draw little (the panel's refreshing power is quoted at
26.4 mW). **MUST** still measure total draw from the Pi with the radio adapter
attached and record it, because extension 5.3 asks for it of every peripheral.

## 4.6 Plan stage exit criteria

Evidence recorded with exact versions. SDK tag and board name pinned. Transport
mechanism confirmed in source. Driver variant and key GPIOs observed. QR
viability established by scanning. Every `KD` item answered or explicitly open
with the phases it blocks.

---

# Stage two, Execute

Each phase **MUST** begin by rereading Flipper Part 0, in particular 0.13.

Each phase carries all five headings. Automated criteria **MUST** be
reproducible by the author on a clean machine with no Pico attached. Hardware
gates are cleared by the author on the device; **MUST NOT** claim one has
passed or infer one from a green automated run.

## KE1: Foundation and first light

**Work** Repository structure per Part 3. Licence (MIT, `KD2`). The typography
scan (copied from the Flipper repository with provenance). Continuous
integration: scan, host tests, sanitiser build, firmware build against the
pinned SDK. The SDK setup script that installs and pins cmake, ninja, the ARM
toolchain and pico-sdk on this Windows machine and in CI, with every version in
one tracked file. A firmware image that initialises the panel, draws a test
pattern, reads both keys, and shows on the panel which key was pressed and
whether the press was short or long. The required documents from Appendix B,
each saying what it is for.

**Precondition** None. This phase depends on no protocol and no appliance.

**Excluded** Any protocol, transport, QR, session or appliance concern. Nothing
here talks to anything.

**Assumptions to verify** Evaluation 4.1 and 4.3: SDK tag and board name; driver
variant; key GPIOs and polarity; that the vendored driver builds warning clean
under the project's flags or is exempted under the vendored rule with the
reason recorded.

**Tests first** Key classification: a hold under the threshold is short, at or
over it is long, a press while another key is held is ignored, an out of range
key or state value is ignored. Debounce: a bounce shorter than the debounce
interval is not a press.

**Done when, automated** Host tests and scan pass on a machine with no SDK.
Firmware builds in CI from the setup script alone. No allocation in the input
model, proven by the allocation wrappers.

**Hardware gate** The image runs on the author's Pico 2 W, draws, and reports
each key and each press kind on the panel. The driver variant and key GPIOs are
recorded in the evaluation log as observed.

## KE2: Display layouts and refresh policy

**Work** The layout module: every display state rendered into a 400 by 300
bitmap from a supplied state structure, with regions declared so that the
region changed between two states is computable. Fixtures for every status
code, every page, the not connected screen, the incompatible screen, the
diagnostic screen, an error band for every error code, and the delivered count
at 0, 9, 999 and 9999. The refresh policy: which region changes cause a
partial refresh, which a full one, and after how many partials a full one is
forced. Measured on the panel.

**Excluded** QR encoding (`KE5`); the code region is drawn as a placeholder
block of the final size. Transport and protocol (`KE3`, `KE4`).

**Assumptions to verify** Evaluation 4.3: partial refresh works on this panel,
its residue, and its time.

**Tests first** State to layout for every fixture. Unknown status or error
value renders a safe fallback rather than blank or garbage. Long values
truncate rather than overflow. Region change detection: a delivered count
change touches only the count region; a page change touches the code region;
a status change never touches the code region. The forced full refresh
counter.

**Done when, automated** Every fixture renders and the region rules hold.

**Hardware gate** Each state legible on the panel at arm's length in daylight
(`KD8`, layouts accepted against the real geometry). Partial refresh of the
count region does not disturb the code region, observed. Residue after the
measured number of partials is acceptable or the forced full refresh interval
is set accordingly.

## KE3: Protocol library, session and development peer

**Work** Copy `protocol/` and its generator from the Flipper repository at a
recorded commit, with the Flipper's test vectors, and the test that holds this
repository's `protocol.json` equal to the appliance's. The session: handshake
with retry, wholesale replacement, incompatible version, not connected on
drop, no queueing across a disconnection, the two guard flags fixed per 2.4,
the Key1 mapping per 2.3, and diagnostic counters. The input model wired to
the session. Sanitiser build. Fuzz harness. The development peer, copied from
the Flipper repository's `peer/` with provenance, built on the host so this
repository can be developed with no appliance present (Flipper 2.9).

**Excluded** The USB transport (`KE4`). Any interpretation of what a button
means beyond the Key1 choice in 2.3.

**Assumptions to verify** That the Flipper's `session/` cannot be copied as is,
because it includes the Flipper display layout header; the Pico session is
written fresh against the Pico display module, using the Flipper's as
reference. That the copied parser builds warning clean under this repository's
flags.

**Tests first** Every verb round trips against the Flipper's vectors. The
`protocol.json` equality test. Handshake success. `BAD_VERSION` marks the link
incompatible. Disconnect during a message. Reconnect discards local state. A
press that cannot be sent now is dropped and counted, never queued. Every
`BUTTON` carries `foregrounded=1` and `unlocked=1`. `HELLO` carries `locked=0`
and the token `stopbath-pico`. The Key1 mapping: `WIFI` sends `RIGHT_SHORT`,
`GUEST` sends `LEFT_SHORT`, `NONE` and no record send `RIGHT_SHORT`, and no
input to the mapping function can produce anything but those two events. No
allocation in the parse or session path.

**Done when, automated** Everything above passes on a clean machine with no
SDK. Sanitiser clean. Fuzz run finds nothing in a short run. The peer drives
every display state and records every event.

## KE4: USB transport

**Work** The TinyUSB CDC edge in `firmware/`: DTR as port opened and closed,
suspend and resume as cable events, bytes in to the session, bytes out from
it, one main loop that services the transport, the keys and the display.

**Excluded** QR (`KE5`). The appliance (`KE6`).

**Assumptions to verify** Evaluation 4.1 and 4.2: the line ending behaviour,
and how DTR and suspend are observed.

**Tests first** Everything that is not SDK bound is already covered by `KE3`;
this phase adds a host test that the transport glue's decision table (which
observed USB fact maps to which session event) is total and matches the
Flipper's, since the appliance's behaviour was proven against that.

**Done when, automated** The firmware builds. The decision table test passes.

**Hardware gate** Against the development peer on a Linux host or WSL: pull
and reinsert the cable twenty times and restart each side independently, with
no repair step. The USB identity the Pico presents (`lsusb`, `udevadm info`)
recorded in the evaluation log; it feeds `docs/APPLIANCE_HANDOFF.md`.

## KE5: QR rendering

**Work** `lib/qrcodegen` vendored as in the Flipper repository. The QR wrapper:
payload to matrix, matrix to the code region at the module size chosen in
4.4, refused with a distinct error when the payload does not fit. Wired to the
`WIFI` and `GUEST` pages.

**Excluded** Deciding what the payloads contain (the appliance's).

**Assumptions to verify** Evaluation 4.4.

**Tests first** Known payload produces the Flipper repository's published
matrix vectors. Oversized payload refused, not truncated. Page switching with
zero, one and two supplied payloads. The Wi-Fi payload at exactly 53 bytes
renders at the chosen version.

**Done when, automated** Vectors match. The refusal path renders the error
fixture.

**Hardware gate** Scanning succeeds from a representative set of phones at
arm's length in daylight. Record the phones, the module size, the distance
and the lighting.

## KE6: Field prototype against the appliance

**Work** Whatever small changes the first sessions against a real appliance
expose. Nothing speculative.

**Precondition** `docs/APPLIANCE_HANDOFF.md` implemented in the StopBath
repository, at least the device rule, so the appliance can find the Pico.

**Excluded** Everything in Part 9. Any protocol change.

**Tests first** A regression test for each defect found.

**Done when, automated** The suite still passes and each field defect has a
test.

**Hardware gate** A full session presented, started and ended from the Pico
against the real appliance, and the same session from the dashboard with the
Pico attached, with no difference in outcome (the `PE4` gate, for this
peripheral). Termination origin `peripheral` shown on the dashboard. The
photographer's answer to Part 8's first question recorded in `FIELD_NOTES.md`.

---

# Stage three, Verify

Flipper Stage three applies: the per phase reproduction report (exact commands,
toolchain and SDK versions, focused and full test results, sanitiser result,
warning count, firmware size, skipped tests and why, tests requiring author
hardware, gates outstanding), the continuous integration list, and the release
checks. **MUST NOT** declare a phase complete before the author reproduces the
automated criteria on a clean machine.

Added to the release checks: the appliance repository's `HARDWARE_COMPATIBILITY.md`
names this device and the appliance version it was proven against.

---

# Part 5: Security

Flipper Part 5 applies verbatim. In addition:

**MUST NOT** write a payload to the Pico's flash, including through any SDK
logging that persists. The only storage this firmware writes is none.

**MUST** clear the frame buffer's code region and the held record on link drop
and on `READY`, so a passphrase does not survive on the panel or in RAM
longer than it is displayed. An e-paper panel keeps its last image with no
power; **MUST** blank the code region on link drop rather than leave the last
code showing on a device that has been unplugged.

The physical key limit stands (extension 5.5): anyone holding the device while
a session runs can show the join code and end the session.

---

# Part 6: Testing

Flipper Part 6 applies. The pure logic is the largest group and runs on the
host in under a second per test. Nothing spans appliance and device by
default. Physical checks are product tests, recorded in `TESTING.md` as
deviations with the reason when they cannot be automated.

---

# Part 7: First prototype scope

Demonstrated against the development peer first, then against the appliance
at `KE6`:

1. the Pico attaches to a host running the development peer and handshakes
2. the peer sends a status and the Pico renders it
3. the peer sends the `WIFI` page with a 53 byte payload and the Pico renders a
   scannable code
4. Key1 switches to the `GUEST` page and back, each a scannable code
5. Key0 short produces `CENTER_SHORT` at the peer, Key0 long produces
   `CENTER_LONG`
6. a delivered count change redraws the count without disturbing the code
7. disconnect and reconnect restores current state safely and shows not
   connected in between

## Success criteria

The question is the same as the Flipper's: does a small dedicated surface let
the photographer stop handling a phone. The Pico adds a second: does a large,
guest facing code scan more reliably than the Flipper's, and does a stranger
understand what to do with it without being told.

---

# Part 8: Field validation

Flipper Part 8 applies. Added: whether the four second full refresh on a page
switch is acceptable in practice; whether guests notice the flicker; whether
the delivered count is read by anyone; whether Key1 needs a long press
meaning; whether the device wants a stand, an enclosure or a lanyard; and
whether the cable to the Pi is the problem that motivates Part 9.

---

# Part 9: Wireless transport, later

The Pico 2 W has a radio and the appliance has a trusted network. Carrying
protocol version 1 over TCP on that network would remove the cable.

**MUST NOT** begin this until `KE6`'s hardware gate has been cleared. The point
of the cabled build is to prove the interaction; the radio adds a transport
question and a security question on top of an unproven device.

It needs appliance side work that is not agreed: a `PeripheralLinkController`
over a network socket, a configuration key for the listen address, and an
answer to the question extension 5.5 forbids answering here: a USB peripheral
is a physical key, while a network peripheral is reachable by anything on the
trusted network that can speak the protocol, so the threat model widens and
the appliance must decide how a network peer proves it is the photographer's
device. The shape of that is sketched, not specified, in
`docs/APPLIANCE_HANDOFF.md` Part 3, and decided in the StopBath repository
(`KD10`).

---

# Part 10: Blocking decisions

**MUST NOT** guess any of these. Ask in the format required by Flipper 0.3,
batched by the stage that needs them.

## Settled 2026-09-15

`KD1` **Language, toolchain and transport. Settled: C on pico-sdk, cmake and
ninja with the ARM GNU toolchain at the version the pinned SDK expects, installed
by a tracked script; USB CDC transport.** The Flipper's proven, fuzzed protocol
library is reused; host tests run under the existing `gcc` and `make` pattern;
the C rules apply unchanged.

`KD2` **Identifier prefix, repository name, licence. Settled: `KE` and `KD`,
`stopbath-pico`, MIT.**

`KD3` **Plan form. Settled: this companion specification, kept short by
reference to the Flipper's.**

`KD4` **Key1 mapping. Settled for the proving phase as 2.3 states.** Revisited
as `KD9` once proven on hardware.

`KD5` **Screen lock. Settled: none in the first version.** Flags per 2.4. Key1
long press reserved.

`KD6` **Toolchain route. Settled: the manual, pinned, scripted install rather
than the VS Code extension**, so continuous integration runs the same script.

`KD7` **Peripheral token and appliance device rule. Settled: token
`stopbath-pico`; the rule via a generalisation of the appliance's script 47,
specified in `docs/APPLIANCE_HANDOFF.md` and implemented in the StopBath
repository.**

## Open

`KD8` **Display layouts.** Real layouts against the panel, replacing the
sketches in 2.6, accepted at the `KE2` hardware gate.

`KD9` **Aligning Key1 after the proving phase.** Whether the mapping in 2.3
stands, whether Key1 gains a long press meaning, or whether the appliance
offers a relative page event in a protocol version 2. Blocks nothing before
`KE6`. Decided with the field evidence, in both repositories.

`KD10` **Wireless transport.** Part 9. Blocks nothing in this document's
phases. Decided in the StopBath repository.

`KD11` **The short and long press threshold and the debounce interval.**
Appendix A gives starting points; settled by field use after `KE1`.

## Recorded as guidance, not decisions

The development peer is copied from the Flipper repository rather than
referenced across repositories, because Flipper 2.9 requires a peer in the
repository that is developed against it and the two peers must not drift
apart from the one protocol. If the Flipper's changes, the copy's provenance
record says which commit this one came from.

---

# Appendix A: Unverified values

None is an accepted contract. **MUST** replace each with a measured or
confirmed value, or an explicit author decision, before the phase that depends
on it.

| Value | Starting point | Settled by |
|---|---|---|
| pico-sdk tag | 2.3.1, the latest release tag listed on 2026-09-15 | 4.1, `KE1` |
| ARM toolchain version | whatever the pinned SDK documents | 4.1, `KE1` |
| Board name in the SDK | `pico2_w` | 4.1, `KE1` |
| Display driver variant | `EPD_4in2` or `EPD_4in2_V2` | 4.3, `KE1` gate |
| Key GPIOs | GP2 and GP3, or GP15 and GP17, evidence conflicts | 4.3, `KE1` gate |
| Key polarity | active low with pull up, unverified | 4.3, `KE1` gate |
| Long press threshold | 1000 ms (`STOPBATH_SPEC.md` 5.4 guidance) | `KD11` |
| Debounce interval | 20 ms | `KD11` |
| Full refresh time | 4 s, manufacturer's figure | 4.3, `KE2` gate |
| Partial refresh time and residue | unmeasured | 4.3, `KE2` gate |
| Partials before a forced full refresh | 5, from the manufacturer's FAQ | `KE2` gate |
| QR version, error correction, module size | version 3, low, 8 px per module | 4.4, `KE5` gate |
| USB vendor and product | 2E8A and 0009, from the SDK descriptor source | 4.2, `KE4` gate |
| USB serial string stability | believed stable (flash unique id), unobserved | 4.2, `KE4` gate |
| Line ending translation in `stdio_usb` | believed on by default, unverified | 4.1, `KE4` |
| Total draw beside the radio adapter | unmeasured | 4.5, `KE4` gate |

# Appendix B: Required repository documents

`README.md`, `PROTOCOL.md` referencing the definition in the StopBath
repository, `TESTING.md`, `IMPLEMENTATION_DEVIATIONS.md`,
`HARDWARE_COMPATIBILITY.md`, `FIELD_NOTES.md`,
`docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md`, `docs/APPLIANCE_HANDOFF.md`,
and `LICENSE`.
