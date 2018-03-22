#include "spi.h"
#include "rs232.h"

#include <string.h>

#define SPI_SEGMENT_MAXLEN 32
#define SPI_REPLY_MAXLEN 32

int SPI_Write(HCOM hCom, const void* buf, size_t count, char flag) {
	char rx_buf[SPI_REPLY_MAXLEN];
	const char* tx_buf = (char*)buf;
	unsigned char header[2];
	header[0] = 'S';
	header[1] = SPI_SEGMENT_MAXLEN | SPI_NO_STOP;

	while (count > SPI_SEGMENT_MAXLEN) {	
		if (RS232_Write(hCom, header, 2) == -1)
			goto err;
		
		if (RS232_Write(hCom, tx_buf, SPI_SEGMENT_MAXLEN) == -1)
			goto err;

		if (RS232_ReadLine(hCom, rx_buf, SPI_REPLY_MAXLEN) == -1)
			goto err;

		if (!strcmp(rx_buf, "OK"))
			goto err;

		tx_buf += SPI_SEGMENT_MAXLEN;
		count -= SPI_SEGMENT_MAXLEN;
	}

	if (count > 0) {
		if (flag & SPI_NO_STOP) {
			header[1] = count | SPI_NO_STOP;
		} else {
			header[1] = count;
		}
		
		if (RS232_Write(hCom, header, 2) == -1)
			goto err;

		if (RS232_Write(hCom, tx_buf, count) == -1)
			goto err;

		if (RS232_ReadLine(hCom, rx_buf, SPI_REPLY_MAXLEN) == -1)
			goto err;

		if (!strcmp(rx_buf, "OK"))
			goto err;
	}

	return 0;
err:
	return -1;
}

int SPI_Read(HCOM hCom, void* buf, size_t count, char flag) {
	char* rx_buf = (char*)buf;
	unsigned char header[2];
	size_t read;
	header[0] = 'G';
	header[1] = SPI_SEGMENT_MAXLEN | SPI_NO_STOP;

	while (count > SPI_SEGMENT_MAXLEN) {
		if (RS232_Write(hCom, header, 2) == -1)
			goto err;

		read = 0;
		do {
			size_t rs232_read_ret = RS232_Read(hCom, rx_buf, SPI_SEGMENT_MAXLEN - read);
			if (rs232_read_ret == -1)
				goto err;
			read += rs232_read_ret;
			rx_buf += rs232_read_ret;
		} while (read < SPI_SEGMENT_MAXLEN);

		count -= SPI_SEGMENT_MAXLEN;
	}

	if (count > 0) {
		if (flag & SPI_NO_STOP) {
			header[1] = count | SPI_NO_STOP;
		} else {
			header[1] = count;
		}

		if (RS232_Write(hCom, header, 2) == -1)
			goto err;

		read = 0;
		do {
			size_t rs232_read_ret = RS232_Read(hCom, rx_buf, count);
			if (rs232_read_ret == -1)
				goto err;
			read += rs232_read_ret;
			rx_buf += rs232_read_ret;
		} while (read < count);

		return 0;
	}

	return 0;
err:
	return -1;
}

int SPI_CS(HCOM hCom, unsigned char val) {
	unsigned char header[2];
	if (val) {
		header[0] = 'G';
		header[1] = 0;
	} else {
		header[0] = 'S';
		header[1] = 0;
	}

	return (RS232_Write(hCom, header, 2) == -1) ? -1 : 0;
}
