#include "flash25.h"
#include "spi.h"
#include "rs232.h"

#include <stdint.h>

uint32_t SPI_Flash25_ReadID(HCOM hCom) {
	uint8_t cmd = FLASH25_CMD_RDID;
	uint8_t id[3] = { 0 };
	SPI_Write(hCom, &cmd, 1, SPI_NO_STOP);
	SPI_Read(hCom, &id, 3, SPI_STOP_AFTER);

	return (id[2] << 16) | (id[1] << 8) | id[0];
}

void SPI_Flash25_WriteEnable(HCOM hCom) {
	unsigned char cmd = FLASH25_CMD_WREN;
	SPI_Write(hCom, &cmd, 1, SPI_STOP_AFTER);
}

void SPI_Flash25_WaitForWriteEnd(HCOM hCom) {
	uint8_t temp = FLASH25_CMD_RDSR;
	SPI_Write(hCom, &temp, 1, SPI_NO_STOP);

	/* Loop as long as the memory is busy with a write cycle */
	do {
		/* Send a dummy byte to generate the clock needed by the FLASH
		and put the value of the status register in FLASH_Status variable */
		SPI_Read(hCom, &temp, 1, SPI_NO_STOP);

	} while (temp & FLASH25_FLAG_WIP);

	// Set CS to high
	SPI_CS(hCom, 1);
}

static void SPI_Flash25_PageWrite(HCOM hCom, uint8_t* buf, uint32_t addr, uint32_t count) {
	/* Enable the write access to the FLASH */
	SPI_Flash25_WriteEnable(hCom);

	unsigned char header[4] = { 0 };
	header[0] = FLASH25_CMD_WRITE;
	header[1] = (addr & 0xFF0000) >> 16;
	header[2] = (addr & 0xFF00) >> 8;
	header[3] = addr & 0xFF;
	
	SPI_Write(hCom, header, 4, SPI_NO_STOP);
	SPI_Write(hCom, buf, count, SPI_STOP_AFTER);

	/* Wait the end of Flash writing */
	SPI_Flash25_WaitForWriteEnd(hCom);
}

void SPI_Flash25_BufferWrite(HCOM hCom, uint8_t* buf, uint32_t addr, uint32_t page_size, uint32_t count) {
	uint32_t page_count = 0, byte_remaining = 0, offset = 0, byte_to_boundary = 0;

	offset = addr % page_size;
	byte_to_boundary = page_size - offset;
	page_count = count / page_size;
	byte_remaining = count % page_size;

	if (offset == 0) { /* addr is page_size aligned  */
		if (page_count == 0) { /* byte_to_boundary < page_size */
			SPI_Flash25_PageWrite(hCom, buf, addr, count);
		} else { /* byte_to_boundary > page_size */
			while (page_count--) {
				SPI_Flash25_PageWrite(hCom, buf, addr, page_size);
				addr += page_size;
				buf += page_size;
			}

			SPI_Flash25_PageWrite(hCom, buf, addr, byte_remaining);
		}
	} else { /* addr is not page_size aligned  */
		if (page_count == 0) { /* byte_to_boundary < page_size */
			if (byte_remaining > byte_to_boundary) { /* (byte_to_boundary + addr) > page_size */
				byte_remaining -= byte_to_boundary;

				SPI_Flash25_PageWrite(hCom, buf, addr, byte_to_boundary);
				addr += byte_to_boundary;
				buf += byte_to_boundary;

				SPI_Flash25_PageWrite(hCom, buf, addr, byte_remaining);
			} else {
				SPI_Flash25_PageWrite(hCom, buf, addr, count);
			}
		} else { /* more than 1 page */
			count -= byte_to_boundary;
			page_count = count / page_size;
			byte_remaining = count % page_size;

			SPI_Flash25_PageWrite(hCom, buf, addr, byte_to_boundary);
			addr += byte_to_boundary;
			buf += byte_to_boundary;

			while (page_count--) {
				SPI_Flash25_PageWrite(hCom, buf, addr, page_size);
				addr += page_size;
				buf += page_size;
			}

			if (byte_remaining != 0) {
				SPI_Flash25_PageWrite(hCom, buf, addr, byte_remaining);
			}
		}
	}
}

void SPI_Flash25_BufferRead(HCOM hCom, uint8_t* pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead) {
	uint8_t header[4];
	header[0] = FLASH25_CMD_READ;
	header[1] = (ReadAddr & 0xFF0000) >> 16;
	header[2] = (ReadAddr & 0xFF00) >> 8;
	header[3] = ReadAddr & 0xFF;

	SPI_Write(hCom, header, 4, SPI_NO_STOP);
	SPI_Read(hCom, pBuffer, NumByteToRead, SPI_STOP_AFTER);
}

void SPI_Flash25_SectorErase(HCOM hCom, uint32_t SectorAddr) {
	/* Send write enable instruction */
	SPI_Flash25_WriteEnable(hCom);

	uint8_t header[4];
	header[0] = FLASH25_CMD_SE;
	header[1] = (SectorAddr & 0xFF0000) >> 16;
	header[2] = (SectorAddr & 0xFF00) >> 8;
	header[3] = SectorAddr & 0xFF;

	SPI_Write(hCom, header, 4, SPI_STOP_AFTER);

	/* Wait the end of Flash writing */
	SPI_Flash25_WaitForWriteEnd(hCom);
}

void SPI_Flash25_BulkErase(HCOM hCom) {
	/* Send write enable instruction */
	SPI_Flash25_WriteEnable(hCom);

	unsigned char cmd = FLASH25_CMD_BE;
	SPI_Write(hCom, &cmd, 1, SPI_STOP_AFTER);

	/* Wait the end of Flash writing */
	SPI_Flash25_WaitForWriteEnd(hCom);
}