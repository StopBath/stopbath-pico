#include "DEV_Config.h"

#include "hardware/gpio.h"

/* The panel is on SPI1: the module wires CLK to GP10 and DIN to GP11, which
 * the RP2350 pin multiplexer offers as SPI1 SCLK and SPI1 TX (evaluation log
 * 4.3). The board's default SPI (SPI0 on GP16 to GP19) is not the panel's. */
#define PANEL_SPI spi1

/* Upstream's rate. The panel's datasheet limit is not verified; a faster
 * rate is a KE2 measurement, since the frame takes 15000 bytes per refresh. */
#define PANEL_SPI_BAUDRATE_HZ (4000 * 1000)

int EPD_RST_PIN = 12;
int EPD_DC_PIN = 8;
int EPD_CS_PIN = 9;
int EPD_BUSY_PIN = 13;
int EPD_CLK_PIN = 10;
int EPD_MOSI_PIN = 11;

void DEV_Digital_Write(UWORD Pin, UBYTE Value) {
    gpio_put(Pin, Value != 0);
}

UBYTE DEV_Digital_Read(UWORD Pin) {
    return gpio_get(Pin) ? 1 : 0;
}

void DEV_SPI_WriteByte(UBYTE Value) {
    spi_write_blocking(PANEL_SPI, &Value, 1);
}

void DEV_Delay_ms(UDOUBLE xms) {
    sleep_ms(xms);
}

static void configure_output(int pin, bool initial_level) {
    gpio_init((uint)pin);
    gpio_set_dir((uint)pin, true);
    gpio_put((uint)pin, initial_level);
}

UBYTE DEV_Module_Init(void) {
    configure_output(EPD_RST_PIN, true);
    configure_output(EPD_DC_PIN, false);
    /* Chip select idles high: the drivers pull it low around each byte. */
    configure_output(EPD_CS_PIN, true);
    gpio_init((uint)EPD_BUSY_PIN);
    gpio_set_dir((uint)EPD_BUSY_PIN, false);

    spi_init(PANEL_SPI, PANEL_SPI_BAUDRATE_HZ);
    gpio_set_function((uint)EPD_CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function((uint)EPD_MOSI_PIN, GPIO_FUNC_SPI);
    return 0;
}

void DEV_Module_Exit(void) {
}
