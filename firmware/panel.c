#include "panel.h"

#include "EPD_4in2_V2.h"

void panel_initialise(void) {
    EPD_4IN2_V2_Init();
}

void panel_show_full_frame(const uint8_t* frame) {
    /* The driver takes a mutable pointer but only reads; the cast keeps this
     * interface honest about the frame being the caller's. */
    EPD_4IN2_V2_Display((UBYTE*)(uintptr_t)frame);
}

void panel_sleep(void) {
    EPD_4IN2_V2_Sleep();
}

const char* panel_driver_name(void) {
    return "EPD_4IN2_V2";
}
