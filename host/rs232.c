#include "rs232.h"

ssize_t RS232_ReadLine(HCOM hCom, char *buffer, size_t count) {
	size_t received = 0;
	ssize_t n = 0;
	char sbuf;

	do {
		n = RS232_Read(hCom, &sbuf, 1);

		if (n == -1)
			return -1;

		buffer[received] = sbuf;
		received += n;
	} while (sbuf != '\n' && (received + 2) < count);

	if (buffer[received - 2] == '\r')
		received--;
	buffer[received] = '\0';
	return received;
}

#ifdef _WIN32
HCOM RS232_Open(const unsigned char* name) {
	HCOM hCom = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
	
	return hCom;
}

void RS232_Config(HCOM hCom, speed_t baud, unsigned char config) {
	DCB dcb = { 0 };
	GetCommState(hCom, &dcb);

	dcb.BaudRate = baud;

	switch (config & RS232_CONFIG_CS_MASK) {
	case RS232_CONFIG_CS5:
		dcb.ByteSize = 5;
		break;
	case RS232_CONFIG_CS6:
		dcb.ByteSize = 6;
		break;
	case RS232_CONFIG_CS7:
		dcb.ByteSize = 7;
		break;
	case RS232_CONFIG_CS8:
	default:
		dcb.ByteSize = 8;
		break;
	}

	switch (config & RS232_CONFIG_PARITY_MASK) {
	case RS232_CONFIG_PARITY_EVEN:
		dcb.ByteSize = EVENPARITY;
		break;
	case RS232_CONFIG_PARITY_ODD:
		dcb.Parity = ODDPARITY;
		break;
	case RS232_CONFIG_PARITY_NO:
	default:
		dcb.Parity = NOPARITY;
		break;
	}

	dcb.StopBits = (config & RS232_CONFIG_STOPBIT_2) ? TWOSTOPBITS : ONESTOPBIT;
	dcb.fOutxCtsFlow = 0;
	dcb.fOutxDsrFlow = 0;
	dcb.fDsrSensitivity = 0;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;

	SetCommState(hCom, &dcb);
}

ssize_t RS232_Write(HCOM hCom, const void* buf, size_t count) {
	DWORD written = 0;
	if (WriteFile(hCom, buf, count, &written, NULL) == FALSE) {
		return -1;	// Error
	} else {
		return written;
	}
}

ssize_t RS232_Read(HCOM hCom, void *buf, size_t count) {
	DWORD read = 0;
	if (ReadFile(hCom, buf, count, &read, NULL) == FALSE) {
		return -1;	// Error
	} else {
		return read;
	}
}

void RS232_RTS_Set(HCOM hCom) {
	EscapeCommFunction(hCom, SETRTS);
}

void RS232_RTS_Clr(HCOM hCom) {
	EscapeCommFunction(hCom, CLRRTS);
}

void RS232_DTR_Set(HCOM hCom) {
	EscapeCommFunction(hCom, SETDTR);
}

void RS232_DTR_Clr(HCOM hCom) {
	EscapeCommFunction(hCom, CLRDTR);
}

void RS232_Close(HCOM hCom) {
	CloseHandle(hCom);
}

int RS232_CTS(HCOM hCom) {
	DWORD flags;
	GetCommModemStatus(hCom, &flags);
	return (flags & MS_CTS_ON) ? 1 : 0;
}

int RS232_DSR(HCOM hCom) {
	DWORD flags;
	GetCommModemStatus(hCom, &flags);
	return (flags & MS_DSR_ON) ? 1 : 0;
}
#else
HCOM RS232_Open(const unsigned char* name) {
	return open(name, O_RDWR | O_NOCTTY);
}

void RS232_Config(HCOM fd, speed_t baud, unsigned char config) {
	struct termios options;
	int baudr;
	tcgetattr(fd, &options);

	switch (baud) {
	case 50: 
		baudr = B50;
		break;
	case 75: 
		baudr = B75;
		break;
	case 110: 
		baudr = B110;
		break;
	case 134: 
		baudr = B134;
		break;
	case 150: 
		baudr = B150;
		break;
	case 200: 
		baudr = B200;
		break;
	case 300: 
		baudr = B300;
		break;
	case 600: 
		baudr = B600;
		break;
	case 1200: 
		baudr = B1200;
		break;
	case 1800: 
		baudr = B1800;
		break;
	case 2400: 
		baudr = B2400;
		break;
	case 4800: 
		baudr = B4800;
		break;
	case 9600: 
		baudr = B9600;
		break;
	case 19200: 
		baudr = B19200;
		break;
	case 38400: 
		baudr = B38400;
		break;
	case 57600: 
		baudr = B57600;
		break;
	case 115200: 
		baudr = B115200;
		break;
	case 230400: 
		baudr = B230400;
		break;
	case 460800: 
		baudr = B460800;
		break;
	case 500000: 
		baudr = B500000;
		break;
	case 576000: 
		baudr = B576000;
		break;
	case 921600: 
		baudr = B921600;
		break;
	case 1000000: 
		baudr = B1000000;
		break;
	case 1152000: 
		baudr = B1152000;
		break;
	case 1500000: 
		baudr = B1500000;
		break;
	case 2000000: 
		baudr = B2000000;
		break;
	case 2500000: 
		baudr = B2500000;
		break;
	case 3000000: 
		baudr = B3000000;
		break;
	case 3500000: 
		baudr = B3500000;
		break;
	case 4000000: 
		baudr = B4000000;
		break;
	default: 
		baudr = B1152000;
		break;
	}
	cfsetispeed(&options, baudr);
	cfsetospeed(&options, baudr);

	options.c_cflag &= ~CSIZE;
	switch (config & RS232_CONFIG_CS_MASK) {
	case RS232_CONFIG_CS5:
		options.c_cflag |= CS5;
		break;
	case RS232_CONFIG_CS6:
		options.c_cflag |= CS6;
		break;
	case RS232_CONFIG_CS7:
		options.c_cflag |= CS7;
		break;
	case RS232_CONFIG_CS8:
	default:
		options.c_cflag |= CS8;
		break;
	}

	switch (config & RS232_CONFIG_PARITY_MASK) {
	case RS232_CONFIG_PARITY_EVEN:
		options.c_cflag |= PARENB;
		options.c_cflag &= ~PARODD;
		break;
	case RS232_CONFIG_PARITY_ODD:
		options.c_cflag |= PARENB;
		options.c_cflag |= PARODD;
		break;
	case RS232_CONFIG_PARITY_NO:
	default:
		options.c_cflag &= ~PARENB;
		break;
	}

	if (config & RS232_CONFIG_STOPBIT_2) {
		options.c_cflag |= CSTOPB;
	} else {
		options.c_cflag &= ~CSTOPB;
	}

	options.c_cflag &= ~CRTSCTS;
	options.c_cflag |= CREAD | CLOCAL;

	// Put the serial port in raw mode, similar to cfmakeraw
	options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	options.c_oflag &= ~OPOST;
	options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tcsetattr(fd, TCSANOW, &options);
}

ssize_t RS232_Write(HCOM fd, const void* buf, size_t count) {
	return write(fd, buf, count);
}

ssize_t RS232_Read(HCOM fd, void *buf, size_t count) {
	return read(fd, buf, count);
}

void RS232_RTS_Set(HCOM fd) {
	int RTS_flag = TIOCM_RTS;
	ioctl(fd, TIOCMBIS, &RTS_flag);
}

void RS232_RTS_Clr(HCOM fd) {
	int RTS_flag = TIOCM_RTS;
	ioctl(fd, TIOCMBIC, &RTS_flag);
}

void RS232_DTR_Set(HCOM fd) {
	int DTR_flag = TIOCM_DTR;
	ioctl(fd, TIOCMBIS, &DTR_flag);
}

void RS232_DTR_Clr(HCOM fd) {
	int DTR_flag = TIOCM_DTR;
	ioctl(fd, TIOCMBIC, &DTR_flag);
}

int RS232_CTS(HCOM fd) {
	int flags;
	ioctl(fd, TIOCMGET, &flags);
	return (flags & TIOCM_CTS) ? 1 : 0;
}

int RS232_DSR(HCOM fd) {
	int flags;
	ioctl(fd, TIOCMGET, &flags);
	return (flags & TIOCM_DSR) ? 1 : 0;
}

void RS232_Close(HCOM hCom) {
	close(hCom);
}
#endif


