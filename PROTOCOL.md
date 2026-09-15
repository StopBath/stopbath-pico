# Protocol

This repository implements the StopBath peripheral protocol, version 1. It
does not define it.

The definition is owned by the StopBath repository (extension 4.1) and lives
there in `docs/peripheral/PROTOCOL.md` (the prose) and
`docs/peripheral/protocol.json` (the single normative table). It was frozen
at `FD20` on 2026-09-12 and is a sacred contract under Flipper 0.11: nothing
in it is renamed, aliased, normalised or improved by an implementer.

## What this repository will hold

From `KE3`: a copy of `protocol.json`, a test that holds it equal to the
appliance's, the parser and encoder copied from the `stopbath-flipper`
repository with provenance (they are generated from that table and tested
against its vectors), and a `session/` that speaks it. Until `KE3` there is
no protocol code here at all; `KE1` and `KE2` talk to nothing (spec Part 3,
Flipper 2.9).

## What this device sends

| Verb | Fields this device sends | Note |
|---|---|---|
| `HELLO` | `version=1 peripheral=stopbath-pico locked=0` | on attach and restart (`KD7`) |
| `BUTTON` | `event=<one of the four> foregrounded=1 unlocked=1` | both flags always true, spec 2.4 |
| `STATE` | never | there is no lock, `KD5` |

Which event Key1 sends is decided by the page in the last `DISPLAY` received
(spec 2.3, a recorded deviation, provisional until `KD9`).

## What this repository never proposes

A protocol change. If the field shows one is needed, it is raised in the
StopBath repository as a version 2 decision (spec 2.8, `docs/APPLIANCE_HANDOFF.md`
Part 2).
