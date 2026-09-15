/*
 * How the remote presents itself on the USB bus.
 *
 * The descriptors follow the SDK's own for a CDC device,
 * src/rp2_common/pico_stdio_usb/stdio_usb_descriptors.c at the pinned tag
 * (evaluation log 4.2), field for field: one CDC function on interfaces 0
 * and 1, endpoints 0x81 (notification), 0x02 and 0x82 (data), full speed
 * packets, bus powered. They are written here rather than taken from the
 * SDK's file because that file belongs to pico_stdio_usb, whose stdio this
 * project must not link (evaluation log 4.1, the line ending translation).
 *
 * The identity is Raspberry Pi's vendor id 2E8A with product id 0009, their
 * "Raspberry Pi Pico SDK CDC UART" allocation, which is what this is: a
 * standard CDC device built on the SDK. Raspberry Pi's usb-pid guidance
 * (read 2026-09-15) is that a standard class device needs no product id of
 * its own and should identify itself by its product string, so the product
 * string names the device. The serial string is the flash unique id, which
 * the appliance's udev rule matches (the StopBath repository's docs/PICO_REMOTE_HANDOFF.md 1.1).
 */
#include "pico/unique_id.h"
#include "tusb.h"

#define USBD_VID (0x2E8A)
#define USBD_PID (0x0009)
#define USBD_MANUFACTURER "Raspberry Pi"
#define USBD_PRODUCT "StopBath Pico Remote"

#define USBD_DESC_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)
#define USBD_MAX_POWER_MA (250)

#define USBD_ITF_CDC (0)
#define USBD_ITF_MAX (2)

#define USBD_CDC_EP_CMD (0x81)
#define USBD_CDC_EP_OUT (0x02)
#define USBD_CDC_EP_IN (0x82)
#define USBD_CDC_CMD_MAX_SIZE (8)
#define USBD_CDC_IN_OUT_MAX_SIZE (64)

#define USBD_STR_0 (0x00)
#define USBD_STR_MANUF (0x01)
#define USBD_STR_PRODUCT (0x02)
#define USBD_STR_SERIAL (0x03)
#define USBD_STR_CDC (0x04)

/* Longest string above plus one, in UTF-16 units; the SDK's default of
 * twenty would cut the product name. */
#define USBD_DESC_STR_MAX (32)

static const tusb_desc_device_t usbd_desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USBD_VID,
    .idProduct = USBD_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = USBD_STR_MANUF,
    .iProduct = USBD_STR_PRODUCT,
    .iSerialNumber = USBD_STR_SERIAL,
    .bNumConfigurations = 1,
};

static const uint8_t usbd_desc_cfg[USBD_DESC_LEN] = {
    TUD_CONFIG_DESCRIPTOR(1, USBD_ITF_MAX, USBD_STR_0, USBD_DESC_LEN, 0, USBD_MAX_POWER_MA),
    TUD_CDC_DESCRIPTOR(
        USBD_ITF_CDC,
        USBD_STR_CDC,
        USBD_CDC_EP_CMD,
        USBD_CDC_CMD_MAX_SIZE,
        USBD_CDC_EP_OUT,
        USBD_CDC_EP_IN,
        USBD_CDC_IN_OUT_MAX_SIZE),
};

static char usbd_serial_str[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];

static const char* const usbd_desc_str[] = {
    [USBD_STR_MANUF] = USBD_MANUFACTURER,
    [USBD_STR_PRODUCT] = USBD_PRODUCT,
    [USBD_STR_SERIAL] = usbd_serial_str,
    [USBD_STR_CDC] = "StopBath peripheral link",
};

const uint8_t* tud_descriptor_device_cb(void) {
    return (const uint8_t*)&usbd_desc_device;
}

const uint8_t* tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return usbd_desc_cfg;
}

const uint16_t* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    static uint16_t desc_str[USBD_DESC_STR_MAX];
    if(!usbd_serial_str[0]) {
        pico_get_unique_board_id_string(usbd_serial_str, sizeof(usbd_serial_str));
    }
    uint8_t length;
    if(index == 0) {
        desc_str[1] = 0x0409;
        length = 1;
    } else {
        if(index >= sizeof(usbd_desc_str) / sizeof(usbd_desc_str[0])) {
            return NULL;
        }
        const char* text = usbd_desc_str[index];
        for(length = 0; length < USBD_DESC_STR_MAX - 1 && text[length]; ++length) {
            desc_str[1 + length] = (uint16_t)text[length];
        }
    }
    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * length + 2));
    return desc_str;
}
