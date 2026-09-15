# Provenance: the QR encoder

Vendored verbatim, unmodified, from Project Nayuki's QR Code generator, the C
implementation. Not to be edited: a fix goes upstream or into the wrapper in
`remote_display/remote_qr.c` (Flipper 0.12, applied here by Pico spec Part 0).

| Field | Value |
|---|---|
| Source | https://github.com/nayuki/QR-Code-generator |
| Pinned at | tag `v1.8.0`, commit `720f62bddb7226106071d4728c292cb1df519ceb` (2022-04-17) |
| Files | `c/qrcodegen.c`, `c/qrcodegen.h`; the MIT text from the upstream readme as `LICENSE.txt` |
| Licence | MIT, the same as this repository |
| Copied from | the `stopbath-flipper` repository's `lib/qrcodegen/` at commit `3677e61904d854504ac58c5dce6d6e6419836cfc`, 2026-09-15, byte for byte; that repository's evaluation log carries the dependency record (behaviour, maintenance, allocation, security, replacement cost) made under Flipper 0.12 on 2026-09-11 |

| File | sha256 |
|---|---|
| `qrcodegen.c` | `9b2f2f9dc36dc1804be715b729d820fbfa26a8e4e1aa5316511c1971aaebb58a` |
| `qrcodegen.h` | `26e76f083ad582f246c5a4a002e34eaeb55e2f96a065552b6455424b04a03519` |
| `LICENSE.txt` | `527e09cc1cadbe29d0368a41e7fec6f728a19b03b3cb08839205a18a0d267166` |

## What this project relies on

`qrcodegen_encodeText` with caller supplied buffers sized by
`qrcodegen_BUFFER_LEN_FOR_VERSION` for the ceiling version in
`remote_display/remote_qr.h`; no heap allocation anywhere in the library
(its own header says so, and the Flipper repository confirmed it by grep). It
refuses, rather than overruns, a text that does not fit the version range.
The payload it consumes is the appliance's and is untrusted on the same terms
as the rest of the protocol.

## Verifying

```bash
cd lib/qrcodegen && sha256sum -c <<'SUMS'
9b2f2f9dc36dc1804be715b729d820fbfa26a8e4e1aa5316511c1971aaebb58a *qrcodegen.c
26e76f083ad582f246c5a4a002e34eaeb55e2f96a065552b6455424b04a03519 *qrcodegen.h
527e09cc1cadbe29d0368a41e7fec6f728a19b03b3cb08839205a18a0d267166 *LICENSE.txt
SUMS
```
