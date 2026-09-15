/*
 * The hardware layer the vendored Waveshare drivers include.
 *
 * Waveshare's drivers (lib/waveshare) are written against this header's
 * names, which Waveshare designed as the porting seam ("we package the
 * hardware layer for easily porting", their wiki). Their own DEV_Config.c is
 * not vendored: it starts the SDK's stdio, whose line ending translation
 * would reach the USB channel the protocol will use (evaluation log 4.1),
 * and it selects the SPI pins by a wrong constant. This one provides exactly
 * what the two drivers call and nothing else, so the vendored files stay
 * byte for byte as upstream ships them (lib/waveshare/PROVENANCE.md).
 *
 * The header name and the identifiers are Waveshare's and are kept, because
 * renaming them would mean editing the vendored files. They are the one
 * place this project's naming rules yield to an external contract.
 */
#pragma once

#include <stdint.h>

#include "hardware/spi.h"
#include "pico/stdlib.h"

#define UBYTE uint8_t
#define UWORD uint16_t
#define UDOUBLE uint32_t

/* The drivers read these as globals, so they are globals, set once by
 * DEV_Module_Init from the pin table in the schematic (evaluation log 4.3). */
extern int EPD_RST_PIN;
extern int EPD_DC_PIN;
extern int EPD_CS_PIN;
extern int EPD_BUSY_PIN;
extern int EPD_CLK_PIN;
extern int EPD_MOSI_PIN;

void DEV_Digital_Write(UWORD Pin, UBYTE Value);
UBYTE DEV_Digital_Read(UWORD Pin);
void DEV_SPI_WriteByte(UBYTE Value);
void DEV_Delay_ms(UDOUBLE xms);

/* Configures the six panel pins and the SPI peripheral. Returns 0, as
 * upstream's does; kept for signature compatibility with the drivers'
 * expectations, though neither vendored driver calls it. */
UBYTE DEV_Module_Init(void);
void DEV_Module_Exit(void);
