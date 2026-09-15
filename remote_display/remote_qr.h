/*
 * QR encoding for the code square of the display, on top of the vendored
 * qrcodegen library (lib/qrcodegen/PROVENANCE.md).
 *
 * No SDK dependency, no allocation: every buffer is sized at compile time
 * from the ceiling version, so a payload that would need more is refused
 * rather than truncated or drawn unscannable (Pico spec 2.7, KE5).
 *
 * The ceiling is this panel's, not the encoder's. The code square is 259
 * pixels (REMOTE_LAYOUT_CODE_SIDE), the quiet zone four modules a side as
 * the standard asks, and a module must be a whole number of pixels for the
 * edges to be crisp. The module is therefore the largest whole pixel size at
 * which the symbol and its quiet zone fit, and the ceiling version is the
 * largest at which that size is still the minimum a phone can be expected
 * to resolve at arm's length: four pixels, about 0.85 mm on this panel
 * (0.212 mm a pixel, evaluation log 4.3). Version 9 is 53 modules; with the
 * quiet zone, 61 by 4 pixels is 244, and version 10 (57 modules) would fall
 * to 3 pixels. The four pixel minimum is a starting point: the KE5 gate,
 * scanning with real phones, may raise it, which lowers the ceiling.
 *
 * The appliance's real payloads are far below the ceiling: the Wi-Fi payload
 * measured on the Flipper is 53 bytes (version 3, 7 pixels a module here),
 * and the gallery address 20 (version 1, 8 pixels). Version 9 holds 230
 * bytes in byte mode at the lowest error correction (probed against the
 * encoder, 2026-09-15), which is more than any Wi-Fi payload the standard
 * grammar can produce (a 32 byte SSID and a 63 byte passphrase come to
 * about 120); the protocol's 256 byte payload bound is the parser's, and a
 * payload between 231 and 256 bytes is shown as too big rather than drawn.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "remote_bitmap.h"
#include "remote_display_layout.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REMOTE_QR_MAX_VERSION 9
#define REMOTE_QR_MAX_SIDE (REMOTE_QR_MAX_VERSION * 4 + 17)
#define REMOTE_QR_QUIET_ZONE_MODULES 4
#define REMOTE_QR_MINIMUM_PIXELS_PER_MODULE 4

/* The library's own sizing for the ceiling version. */
#define REMOTE_QR_BUFFER_LENGTH (((REMOTE_QR_MAX_SIDE * REMOTE_QR_MAX_SIDE) + 7) / 8 + 1)

typedef struct {
    /* Modules per side, 21 to REMOTE_QR_MAX_SIDE; zero when nothing is
     * encoded. */
    int side;
    uint8_t encoded[REMOTE_QR_BUFFER_LENGTH];
} RemoteQrMatrix;

/* Encodes the payload at the smallest version that holds it, up to the
 * ceiling. Returns false, leaving the matrix empty, when the payload needs a
 * larger version or is empty. Error correction starts at the lowest level
 * so that capacity is not spent before it has to be, and is raised by the
 * library where a higher level fits in the same version. */
bool remote_qr_encode(const char* payload, RemoteQrMatrix* matrix);

/* Whether remote_qr_encode would succeed for this payload, without keeping
 * the matrix. The layout uses it to decide whether a code page can show its
 * code or must show a distinct error in its place. */
bool remote_qr_can_encode(const char* payload);

/* False outside the symbol or for an empty matrix. */
bool remote_qr_module_is_dark(const RemoteQrMatrix* matrix, int x, int y);

/* The largest whole number of pixels per module at which a symbol of this
 * side, with its quiet zone, fits a square of area_side pixels. May be
 * below the minimum for a side above the ceiling; the caller decides. */
int remote_qr_pixels_per_module(int side, int area_side);

/* Draws the symbol centred in the area at that module size, dark modules
 * only, so the area's white is the quiet zone. An empty matrix draws
 * nothing. Clipped to the panel. */
void remote_qr_draw(const RemoteQrMatrix* matrix, RemoteBitmap* bitmap, RemoteLayoutRectangle area);

#ifdef __cplusplus
}
#endif
