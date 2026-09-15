#include "panel.h"

#include "../remote_display/remote_bitmap.h"
#include "DEV_Config.h"
#include "EPD_4in2_V2.h"

/*
 * Every command byte below is taken from lib/waveshare/EPD_4in2_V2.c at the
 * vendored commit (line numbers as in that file), not from a datasheet: the
 * vendored driver is the evidence that this sequence drives this panel, and
 * the author saw it draw. The driver's own display functions are not called
 * because each ends by spinning on BUSY (EPD_4IN2_V2_ReadBusy, line 115),
 * which is the blocking first light suffered from. Initialisation and sleep
 * are still the driver's: they run when nothing else needs the loop.
 */

/* SendCommand and SendData, lines 90 to 113. */
static void send_command(uint8_t command) {
    DEV_Digital_Write((UWORD)EPD_DC_PIN, 0);
    DEV_Digital_Write((UWORD)EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(command);
    DEV_Digital_Write((UWORD)EPD_CS_PIN, 1);
}

static void send_data(uint8_t data) {
    DEV_Digital_Write((UWORD)EPD_DC_PIN, 1);
    DEV_Digital_Write((UWORD)EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(data);
    DEV_Digital_Write((UWORD)EPD_CS_PIN, 1);
}

/* SetWindows, lines 164 to 179: x in bytes, y in rows, ends inclusive. */
static void set_window(int x_start, int y_start, int x_end_inclusive, int y_end_inclusive) {
    send_command(0x44);
    send_data((uint8_t)((x_start >> 3) & 0xFF));
    send_data((uint8_t)((x_end_inclusive >> 3) & 0xFF));
    send_command(0x45);
    send_data((uint8_t)(y_start & 0xFF));
    send_data((uint8_t)((y_start >> 8) & 0xFF));
    send_data((uint8_t)(y_end_inclusive & 0xFF));
    send_data((uint8_t)((y_end_inclusive >> 8) & 0xFF));
}

/* SetCursor, lines 181 to 190. The partial path (lines 535 to 540) passes
 * the x in bytes directly, the full path (SetCursor(0, 0)) passes zero;
 * both agree for the x this project uses, so one function serves. */
static void set_cursor(int x_bytes, int y) {
    send_command(0x4E);
    send_data((uint8_t)(x_bytes & 0xFF));
    send_command(0x4F);
    send_data((uint8_t)(y & 0xFF));
    send_data((uint8_t)((y >> 8) & 0xFF));
}

void panel_initialise(void) {
    EPD_4IN2_V2_Init();
}

void panel_begin_full_refresh(const uint8_t* frame) {
    /* A partial pass leaves the update control and border registers set
     * for partial mode (lines 522 to 529). Init, lines 233 to 238, is what
     * sets them for a full refresh; those two writes are repeated here so a
     * full refresh after a partial one does not need the blocking reset.
     * Whether that suffices, or the reset is needed too, is a KE2 gate
     * observation (residue after alternating). */
    send_command(0x21);
    send_data(0x40);
    send_data(0x00);
    send_command(0x3C);
    send_data(0x05);
    set_window(0, 0, REMOTE_BITMAP_WIDTH - 1, REMOTE_BITMAP_HEIGHT - 1);
    set_cursor(0, 0);

    /* Display, lines 356 to 380: the frame into both RAM planes, then
     * TurnOnDisplay, lines 128 to 133, less its busy wait. */
    send_command(0x24);
    for(size_t byte_index = 0; byte_index < REMOTE_BITMAP_SIZE_BYTES; byte_index++) {
        send_data(frame[byte_index]);
    }
    send_command(0x26);
    for(size_t byte_index = 0; byte_index < REMOTE_BITMAP_SIZE_BYTES; byte_index++) {
        send_data(frame[byte_index]);
    }
    send_command(0x22);
    send_data(0xF7);
    send_command(0x20);
}

void panel_begin_partial_refresh(const uint8_t* frame, int x, int y, int width, int height) {
    if(width <= 0 || height <= 0 || x < 0 || y < 0 || x + width > REMOTE_BITMAP_WIDTH || y + height > REMOTE_BITMAP_HEIGHT) {
        return;
    }
    /* PartialDisplay, lines 501 to 557, with the byte arithmetic resolved
     * for an aligned rectangle: the upstream function takes exclusive ends
     * and streams a packed rectangle; here the same byte stream is read
     * row by row out of the full frame. */
    int x_start_bytes = x / 8;
    int x_end_bytes_exclusive = (x + width) / 8;
    int y_end_exclusive = y + height;

    send_command(0x3C);
    send_data(0x80);
    send_command(0x21);
    send_data(0x00);
    send_data(0x00);
    send_command(0x3C);
    send_data(0x80);

    send_command(0x44);
    send_data((uint8_t)(x_start_bytes & 0xFF));
    send_data((uint8_t)((x_end_bytes_exclusive - 1) & 0xFF));
    send_command(0x45);
    send_data((uint8_t)(y & 0xFF));
    send_data((uint8_t)((y >> 8) & 0x01));
    send_data((uint8_t)((y_end_exclusive - 1) & 0xFF));
    send_data((uint8_t)(((y_end_exclusive - 1) >> 8) & 0x01));
    set_cursor(x_start_bytes, y);

    send_command(0x24);
    for(int row = y; row < y_end_exclusive; row++) {
        for(int byte_index = x_start_bytes; byte_index < x_end_bytes_exclusive; byte_index++) {
            send_data(frame[(size_t)row * REMOTE_BITMAP_BYTES_PER_ROW + (size_t)byte_index]);
        }
    }
    /* TurnOnDisplay_Partial, lines 144 to 149, less its busy wait. */
    send_command(0x22);
    send_data(0xFF);
    send_command(0x20);
}

bool panel_is_busy(void) {
    /* ReadBusy, line 117: low is idle, high is busy. */
    return DEV_Digital_Read((UWORD)EPD_BUSY_PIN) == 1;
}

void panel_show_full_frame(const uint8_t* frame) {
    panel_begin_full_refresh(frame);
    while(panel_is_busy()) {
        DEV_Delay_ms(10);
    }
}

void panel_sleep(void) {
    EPD_4IN2_V2_Sleep();
}

const char* panel_driver_name(void) {
    return "EPD_4IN2_V2";
}
