# Field notes

Observations from real use, recorded against the questions in Part 8 of the
Flipper specification and the additions in Part 8 of the Pico specification.
None of them is to be answered by design speculation when using the device
would answer it.

Nothing is recorded yet. The first prototype scope (Pico spec Part 7) has not
been demonstrated.

## Questions to answer in the field

From the Flipper specification, as they apply here:

- Does showing the code from the Pico beat using a phone?
- Do subjects scan it reliably, at what distance, in what light?
- Is the display sufficient?
- Are two code pages understandable?
- Is one key to switch pages the right interaction?
- Is a short Key0 press for a new session intuitive?
- Is a long Key0 press for termination both safe and fast?
- Is confirmation necessary or merely irritating?
- Can it be operated without looking once familiar?
- Is cabling acceptable? Would wireless materially improve things?
- What status information is actually useful, and what is noise?
- Does the photographer still reach for the dashboard?
- Would a dedicated device need more buttons or fewer?

Added by the Pico specification:

- Is the four second full refresh on a page switch acceptable in practice?
- Do guests notice the flicker?
- Is the delivered count read by anyone?
- Does Key1 need a long press meaning?
- Does the device want a stand, an enclosure or a lanyard?
- Is the cable to the Pi the problem that motivates a wireless transport?

## Accidental terminations

Every one that happens is recorded here with the date and what the device was
doing, because the guard (`KD5`, Flipper `FD5`) is revisited only on this
evidence.

None recorded.

## Observations

| Date | Observation | Question it bears on |
|---|---|---|
| 2026-09-15 | First sessions against the real appliance, at the desk: a guest phone joined from the Wi-Fi code on the panel and opened the gallery from the panel's address, four photographs arrived and the count rose beside a code that did not move, and the session was ended from the Pico once and from the dashboard once; a session started from the dashboard reached the panel on its own (appliance journal, evaluation log KE6). The photographer's answer, verbatim: "It kicks the shit out of using my phone." | Does showing the code from the Pico beat using a phone? Yes, in the photographer's words. Do subjects scan it reliably? Is one key to switch pages the right interaction? |
| 2026-09-15 | The module's two on board keys are hard to use. The author wants proper buttons: a toggle switch for the session and dedicated buttons for the screens, arcade style if the space can be justified (spec `KD9`). | Is one key to switch pages the right interaction? No. Would a dedicated device need more buttons or fewer? More, and a switch. |
