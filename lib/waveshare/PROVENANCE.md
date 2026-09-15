# Provenance: Waveshare e-paper drivers

Vendored verbatim, byte for byte including the upstream CRLF line endings
(`.gitattributes` exempts them from normalisation so the digests below hold).
Not edited, and not to be edited: a fix goes upstream or into this project's
own hardware layer in `firmware/`, never into these files (Pico spec Part 3,
Flipper 0.12).

| Field | Value |
|---|---|
| Source | https://github.com/waveshare/Pico_ePaper_Code |
| Path in source | `c/lib/e-Paper/` |
| Commit | `c9bcd84db5adf5f085353649a8a5c31492bc5fb8` (2024-10-24) |
| Retrieved | 2026-09-15 |
| Licence | MIT style permission notice in each file header (Waveshare team) |

| File | sha256 |
|---|---|
| `EPD_4in2_V2.c` | `64958dfd6723e108bfb422126f83d41f242a099f663b3e1b9dabdfea75f9bb4a` |
| `EPD_4in2_V2.h` | `a7682911ff6d9b040ab5c5f7d33cd37c0b93707246f5c7c9da17bbbf8012b523` |

## Why this driver

Waveshare ships two drivers for the 4.2 inch black and white panel and the
wiki does not say which the board sold today needs: `EPD_4in2` speaks the
UC8176 command set the wiki's datasheet link names, `EPD_4in2_V2` a different
one. Both were vendored (the `EPD_4in2` pair at sha256
`766f0a7b364873219d5b818926e375bd7c83cf56ef5a181cc02bc7dec2dac990` and
`285a806f1ddfc818301cfa9a94240583faa8fdc53f1767788c5b7238c78fa50d`, same
commit) and a first light image built against each. The author observed on
2026-09-15 that the V2 image drew and the V1 image did not (evaluation log
4.3), so the V1 driver was removed and only V2 remains.

## What they need from this project

The driver includes `DEV_Config.h` and `Debug.h` and calls `DEV_Digital_Write`,
`DEV_Digital_Read`, `DEV_SPI_WriteByte`, `DEV_Delay_ms`, the `EPD_*_PIN`
globals, and the `Debug` macro. Upstream's `DEV_Config.c` is not vendored:
it calls `stdio_init_all`, which would put the SDK's line ending translation
on the USB channel the protocol will use (evaluation log 4.1), and it is the
porting layer Waveshare designed to be replaced. This project's own is
`firmware/DEV_Config.h` and `firmware/DEV_Config.c`.

## Verifying

```bash
cd lib/waveshare && sha256sum -c <<'SUMS'
64958dfd6723e108bfb422126f83d41f242a099f663b3e1b9dabdfea75f9bb4a *EPD_4in2_V2.c
a7682911ff6d9b040ab5c5f7d33cd37c0b93707246f5c7c9da17bbbf8012b523 *EPD_4in2_V2.h
SUMS
```
