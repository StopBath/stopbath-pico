/*
 * TinyUSB configuration for the link: one CDC function, device mode.
 *
 * Modelled on the SDK's own pico_stdio_usb/include/tusb_config.h at the
 * pinned tag (evaluation log 4.2), less the vendor interface it reserves for
 * its reset feature. CFG_TUSB_MCU and CFG_TUSB_OS come from the SDK's
 * TinyUSB build (lib/tinyusb/hw/bsp/rp2040/family.cmake), not from here.
 *
 * The FIFOs hold one whole protocol line (512 bytes) so a full DISPLAY
 * record arriving between two services of the loop is never split by a
 * full buffer; the endpoint buffer is the full speed packet size.
 */
#pragma once

#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE)

#define CFG_TUD_CDC 1
#define CFG_TUD_CDC_RX_BUFSIZE 512
#define CFG_TUD_CDC_TX_BUFSIZE 512
#define CFG_TUD_CDC_EP_BUFSIZE 64

#define CFG_TUD_VENDOR 0
#define CFG_TUD_HID 0
#define CFG_TUD_MSC 0
#define CFG_TUD_MIDI 0
