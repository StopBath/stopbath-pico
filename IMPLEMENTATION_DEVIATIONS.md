# Implementation deviations

Every unavoidable deviation from the specifications is recorded here with a
one line justification and a covering test (Flipper 0.11). Each entry states
the contract, what was done instead, why, and the test that holds it.

## The guard flags are always true

- The contract: Flipper 2.4, which has the peripheral transmit no button
  event while the screen is locked, the display is off, the application is
  backgrounded or another application is running, and carry a foregrounded
  flag and a lock state on the event for the appliance to enforce.
- What was done instead: every `BUTTON` carries `foregrounded=1` and
  `unlocked=1`, `HELLO` carries `locked=0`, and `STATE` is never sent
  (`session/remote_session.c`).
- Why: both flags are honest. The firmware is single purpose: there is no
  other application, no background and no desktop, so any press it sees is
  foregrounded, which is the Flipper's own reasoning for its foregrounded
  flag. There is no lock (`KD5`, settled 2026-09-15): the lock exists for the
  pocket case, and a bare Pico carrying a 4.2 inch panel, cabled to the
  appliance, is not pocketed. Key1 long press is reserved for a lock if the
  device is ever enclosed or wireless.
- What this does not weaken: the appliance still enforces the guard on the
  flags it receives (extension 2.2), so a different or faulty peripheral
  sending a `0` is rejected. Nothing on the appliance was relaxed
  (the StopBath repository's `docs/PICO_REMOTE_HANDOFF.md` 1.2).
- Author decision: `KD5`, 2026-09-15.
- Covering tests: `every press encodes its event with both guard flags true`
  and `opening the port sends hello with the fixed token and no lock` in
  `tests/test_remote_session.c`.

## Key1 chooses between the two page events from the last record

- The contract: Flipper 2.1, the ownership principle: the peripheral emits
  what physically happened and encodes nothing equivalent to a mapping from
  an event to its meaning.
- What was done instead: a Key1 short press sends `LEFT_SHORT` when the last
  `DISPLAY` record named page `GUEST` and `RIGHT_SHORT` otherwise
  (`remote_session_page_event_for_key1`, the one place it lives).
- Why: the appliance's page events are absolute (`LEFT_SHORT` is the Wi-Fi
  page, `RIGHT_SHORT` the gallery page; seam 3.5) and this device has one key
  to move between them. Choosing from the last record received uses what the
  device already holds in order to render (Flipper 2.5), not a belief about
  the session; what it encodes is which of two page events is the other one.
  If the appliance's table ever changed, only page navigation would
  misbehave: the function can produce nothing but those two events, and a
  test passes every value from minus eight to fifteen through it to prove
  that.
- Author decision: `KD4`, 2026-09-15, for the proving phase; revisited as
  `KD9` with field evidence (the StopBath repository's `docs/PICO_REMOTE_HANDOFF.md` Part 2 lists the
  ways it may be aligned).
- Covering tests: `the key1 mapping can only produce a page event` and the
  Key1 rows of `every press encodes its event with both guard flags true` in
  `tests/test_remote_session.c`.

## A PowerShell script is the Windows side link check

- The contract: Pico spec 0.14, one rule set across two languages: C for
  the firmware and the pure logic, Python for the scans and generators,
  shell for setup, and no third.
- What was done instead: `scripts/link_check.ps1`, a PowerShell script that
  opens the remote's COM port with DTR, answers its HELLO with one DISPLAY
  record and prints the BUTTON lines the keys produce.
- Why: the development peer is POSIX and does not build on the Windows
  host, and the terminal to hand (PuTTY) could not be made to send a bare
  line feed or to paste, so the first look at the link (the KE4 gate's step
  one) had no tool. PowerShell opens a serial port with DTR control and an
  exact line ending with nothing to install, which Python here would need
  (`pyserial`). The script is a development check on the author's machine,
  never part of the firmware, the tests or continuous integration, and it
  interprets nothing: the one record it sends is fixture text.
- Author decision requested: whether this stands, or is replaced by a
  Python script once `pyserial` is installed. Recorded 2026-09-15.
- Covering test: none can be automated; it was seen to work on 2026-09-15
  (evaluation log 4.2).

## Waveshare's identifiers are kept in the hardware layer

- The contract: Flipper 0.5, verbose self documenting names, no `buf`,
  `msg` or the like, and no name the SDK does not impose.
- What was done instead: `firmware/DEV_Config.h` and `firmware/DEV_Config.c`
  use Waveshare's names (`DEV_Digital_Write`, `EPD_RST_PIN`, `UBYTE`, and
  the header name itself), and `firmware/Debug.h` defines their `Debug`
  macro.
- Why: the vendored drivers in `lib/waveshare` include that header by name
  and call those functions by name. Renaming them would mean editing the
  vendored files, which `lib/waveshare/PROVENANCE.md` forbids so that the
  digests stay true. The names are an external contract on the same terms
  as an SDK signature, which 0.5 allows.
- What it does not extend to: anything of this project's own. The two files
  contain nothing but that interface, and `firmware/panel.h` is the boundary
  the rest of the firmware uses.
- Author decision requested: whether this stands. Recorded 2026-09-15.
- Covering test: none can be automated (the layer is SDK bound); the
  boundary is held by the fact that no file outside `firmware/` includes
  `DEV_Config.h`, checked by review.

## Two panel drivers were vendored until first light picked one (closed)

- The contract: Flipper 0.12, one dependency for one behaviour, with the
  reasons recorded.
- What was done instead: from 2026-09-15 until the same evening, both of
  Waveshare's drivers for the 4.2 inch black and white panel were vendored
  and two first light images were built, because the manufacturer's wiki and
  demo repository disagree about which the board sold today needs and only
  the panel could settle it.
- Resolution: the author observed that the `EPD_4in2_V2` image drew and the
  `EPD_4in2` image did not (evaluation log 4.3). The V1 driver, its panel
  binding and its image were removed the same evening; one driver remains.
  While V1 was present it needed `-Wno-unused-but-set-variable` (an assigned
  but unread local at its line 612); V2 compiles under the unmodified base
  warning set, so no demotion remains in the build.
- Author decision: the two driver approach was taken by this implementation
  and closed by the author's observation, 2026-09-15.
- Covering test: the firmware build, which now compiles the one vendored
  driver under the base set with warnings as errors.
