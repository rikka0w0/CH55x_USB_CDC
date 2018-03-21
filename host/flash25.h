#ifndef __FLASH25_H
#define __FLASH25_H

#include "rs232.h"
#include <stdint.h>

#define FLASH25_CMD_WRITE      0x02  /* Write to Memory instruction */
#define FLASH25_CMD_WRSR       0x01  /* Write Status Register instruction */
#define FLASH25_CMD_WREN       0x06  /* Write enable instruction */

#define FLASH25_CMD_READ       0x03  /* Read from Memory instruction */
#define FLASH25_CMD_RDSR       0x05  /* Read Status Register instruction  */
#define FLASH25_CMD_RDID       0x9F  /* Read identification */
#define FLASH25_CMD_SE         0xD8  /* Sector Erase instruction */
#define FLASH25_CMD_BE         0xC7  /* Bulk Erase instruction */

#define FLASH25_FLAG_WIP   0x01  /* Write In Progress (WIP) flag */

uint32_t SPI_Flash25_ReadID(HCOM hCom);
void SPI_Flash25_WriteEnable(HCOM hCom);
void SPI_Flash25_WaitForWriteEnd(HCOM hCom);
void SPI_Flash25_BufferWrite(HCOM hCom, uint8_t* pBuffer, uint32_t WriteAddr, uint32_t page_size, uint32_t NumByteToWrite);
void SPI_Flash25_BufferRead(HCOM hCom, uint8_t* pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead);
void SPI_Flash25_SectorErase(HCOM hCom, uint32_t SectorAddr);
void SPI_Flash25_BulkErase(HCOM hCom);

#endif // !__FLASH25_H
