#ifndef __SPI_H_
#define __SPI_H_

#include "ch554_platform.h"

#define SPI_SetCS(cs) (SCS = cs)
#define SPI_SetClockDivider(n) (SPI0_CK_SE = n)

void SPI_SetMasterMode0(void);
void SPI_SetMasterMode3(void);
void SPI_SetMasterIO(void);
void SPI_SetMasterIOFloat(void);
uint8_t SPI_MasterData(uint8_t dat);

#endif
