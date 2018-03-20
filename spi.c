#include "spi.h"

#include "ch554_platform.h"

void SPI_SetMasterMode0(void) {
	SPI0_SETUP = 0;		// Master, MSB first
	SPI0_CTRL = 0x60;	// Mode 0
}

void SPI_SetMasterMode3(void) {
	SPI0_SETUP = 0;     // Master, MSB first
	SPI0_CTRL = 0x68;	// Mode 3
}

void SPI_SetMasterIO(void) {
	P1_MOD_OC &= 0x0F;
	P1_DIR_PU |= 0xB0;	// SCS,MOSI,SCK push-pull
	P1_DIR_PU &= 0xBF;	// MISO floating
}

void SPI_SetMasterIOFloat(void) {
	P1_MOD_OC |= 0xF0;
	P1_DIR_PU &= 0x0F;	// SCS,MOSI,SCK,MISO floating, no pull-up
}

uint8_t SPI_MasterData(uint8_t dat) {
	SPI0_DATA = dat;
	while(S0_FREE == 0);
	return SPI0_DATA;
}
