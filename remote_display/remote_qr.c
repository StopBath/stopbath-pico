#include "remote_qr.h"

#include <string.h>

#include <qrcodegen.h>

bool remote_qr_encode(const char* payload, RemoteQrMatrix* matrix) {
    memset(matrix, 0, sizeof(*matrix));
    if(payload == NULL || payload[0] == '\0') {
        return false;
    }
    /* The library writes before it reads, so neither buffer needs
     * initialising. Both are sized for the ceiling version and live on the
     * stack for the duration of the call, never on the heap. */
    uint8_t scratch[REMOTE_QR_BUFFER_LENGTH];
    bool encoded = qrcodegen_encodeText(
        payload,
        scratch,
        matrix->encoded,
        qrcodegen_Ecc_LOW,
        qrcodegen_VERSION_MIN,
        REMOTE_QR_MAX_VERSION,
        qrcodegen_Mask_AUTO,
        true);
    if(!encoded) {
        memset(matrix, 0, sizeof(*matrix));
        return false;
    }
    matrix->side = qrcodegen_getSize(matrix->encoded);
    return true;
}

bool remote_qr_can_encode(const char* payload) {
    RemoteQrMatrix matrix;
    return remote_qr_encode(payload, &matrix);
}

bool remote_qr_module_is_dark(const RemoteQrMatrix* matrix, int x, int y) {
    if(matrix->side == 0 || x < 0 || y < 0 || x >= matrix->side || y >= matrix->side) {
        return false;
    }
    return qrcodegen_getModule(matrix->encoded, x, y);
}

int remote_qr_pixels_per_module(int side, int area_side) {
    if(side <= 0) {
        return 0;
    }
    return area_side / (side + 2 * REMOTE_QR_QUIET_ZONE_MODULES);
}

void remote_qr_draw(const RemoteQrMatrix* matrix, RemoteBitmap* bitmap, RemoteLayoutRectangle area) {
    if(matrix->side == 0) {
        return;
    }
    int pixels = remote_qr_pixels_per_module(matrix->side, area.width < area.height ? area.width : area.height);
    if(pixels < 1) {
        return;
    }
    /* Centred, so the quiet zone is at least the standard's four modules on
     * every side and any spare pixels are shared out evenly. */
    int symbol_pixels = matrix->side * pixels;
    int origin_x = area.x + (area.width - symbol_pixels) / 2;
    int origin_y = area.y + (area.height - symbol_pixels) / 2;
    for(int module_y = 0; module_y < matrix->side; module_y++) {
        for(int module_x = 0; module_x < matrix->side; module_x++) {
            if(qrcodegen_getModule(matrix->encoded, module_x, module_y)) {
                remote_bitmap_fill_rectangle(
                    bitmap, origin_x + module_x * pixels, origin_y + module_y * pixels, pixels, pixels, RemoteBitmapBlack);
            }
        }
    }
}
