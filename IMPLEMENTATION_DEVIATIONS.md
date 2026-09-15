# Implementation deviations

Every unavoidable deviation from the specifications is recorded here with a
one line justification and a covering test (Flipper 0.11). Each entry states
the contract, what was done instead, why, and the test that holds it.

Two deviations are already decided by the specification itself and will be
entered here with their tests when the code that carries them exists in
`KE3`: the guard flags always true (spec 2.4, `KD5`) and the Key1 mapping
(spec 2.3, `KD4`). They are listed now so nobody is surprised later.

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
