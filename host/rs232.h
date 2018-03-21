#ifndef __RS232_H
#define __RS232_H

#define RS232_CONFIG_STOPBIT_2 0x40
#define RS232_CONFIG_STOPBIT_1 0x00

#define RS232_CONFIG_PARITY_MASK 0x30
#define RS232_CONFIG_PARITY_EVEN 0x20
#define RS232_CONFIG_PARITY_ODD 0x10
#define RS232_CONFIG_PARITY_NO 0x00

#define RS232_CONFIG_CS_MASK 0x0F
#define RS232_CONFIG_CS8 8
#define RS232_CONFIG_CS7 7
#define RS232_CONFIG_CS6 6
#define RS232_CONFIG_CS5 5

#define RS232_CONFIG_DEFAULT (RS232_CONFIG_CS8 | RS232_CONFIG_PARITY_NO | RS232_CONFIG_STOPBIT_1)

#ifdef _WIN32
#include <Windows.h>
typedef SSIZE_T ssize_t;
typedef unsigned int speed_t;
typedef void* HCOM;
#define RS232_IS_INVALID(hCom) (hCom == INVALID_HANDLE_VALUE)
#else
#include <fcntl.h>   	 /* File Control Definitions           */
#include <termios.h>	 /* POSIX Terminal Control Definitions */
#include <unistd.h> 	 /* UNIX Standard Definitions 	       */ 
#include <errno.h>   	 /* ERROR Number Definitions           */
#include <sys/ioctl.h>   /* ioctl()                            */

typedef int HCOM;
#define RS232_IS_INVALID(hCom) (hCom == -1)
#endif

// Valid bauds: 110, 300, 600, 1200, 2400, 4800, 9600, 19200, 38400
//				57600, 115200, 128000, 256000, 500000, 1000000

HCOM RS232_Open(const unsigned char* name);
void RS232_Config(HCOM hCom, speed_t baud, unsigned char config);
ssize_t RS232_Write(HCOM hCom, const void* buf, size_t count);
ssize_t RS232_Read(HCOM hCom, void *buf, size_t count);
ssize_t RS232_ReadLine(HCOM hCom, char *buffer, size_t count);
void RS232_RTS_Set(HCOM hCom);
void RS232_RTS_Clr(HCOM hCom);
void RS232_DTR_Set(HCOM hCom);
void RS232_DTR_Clr(HCOM hCom);
int RS232_CTS(HCOM hCom);
int RS232_DSR(HCOM hCom);
void RS232_Close(HCOM hCom);

#endif // !__RS232_H
