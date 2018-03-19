#ifndef __USB_CDC_H
#define __USB_CDC_H

#include "ch554_platform.h"

#define  SET_LINE_CODING				0X20			// Configures DTE rate, stop-bits, parity, and number-of-character
#define  GET_LINE_CODING				0X21			// This request allows the host to find out the currently configured line coding.
#define  SET_CONTROL_LINE_STATE			0X22			// This request generates RS-232/V.24 style control signals.

#define LINECODING_ADDR 0xCA
#define LINECODING_SIZE 7
extern_xdatabuf(LINECODING_ADDR, LineCoding);
extern uint32_t CDC_Baud;


#define UART_REV_LEN  64

void CDC_InitBaud(void);
void CDC_SetBaud(void);
void usb_poll(void);
void uart_poll(void);

#endif
