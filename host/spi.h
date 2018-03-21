#ifndef __SPI_H
#define __SPI_H

#include "rs232.h"

// SPI_Write and SPI_Read flags
#define SPI_NO_STOP 0x80 /* After this transmission, CS remains low */
#define SPI_STOP_AFTER 0x00 /* After this transmission, CS will be set to high */

/* Write <count> bytes from <buf> to SPI device */
int SPI_Write(HCOM hCom, const void* buf, size_t count, char flag);
/* Read <count> bytes from SPI device to <buf> */
int SPI_Read(HCOM hCom, void* buf, size_t count, char flag);
/* Set the level of CS, normally you don't need this! */
int SPI_CS(HCOM hCom, unsigned char val);

#endif // !__SPI_H
