#include "usb_cdc.h"
#include "usb_endp.h"
#include <string.h>
#include "i2c.h"
#include "spi.h"
#include "bootloader.h"

#include "ch554_platform.h"

//初始化波特率为57600，1停止位，无校验，8数据位。
xdatabuf(LINECODING_ADDR, LineCoding, LINECODING_SIZE);

#define SI5351_ReferenceClock	"26000000"
#define Device_Version			"1.0.1"

// CDC Tx
idata uint8_t  CDC_PutCharBuf[CDC_PUTCHARBUF_LEN];	// The buffer for CDC_PutChar
idata volatile uint8_t CDC_PutCharBuf_Last = 0;		// Point to the last char in the buffer
idata volatile uint8_t CDC_PutCharBuf_First = 0;	// Point to the first char in the buffer
idata volatile uint8_t CDC_Tx_Busy  = 0;
idata volatile uint8_t CDC_Tx_Full = 0;

// CDC Rx
idata volatile uint8_t CDC_Rx_Pending = 0;	// Number of bytes need to be processed before accepting more USB packets
idata volatile uint8_t CDC_Rx_CurPos = 0;

// CDC configuration
extern uint8_t UsbConfig;
uint32_t CDC_Baud = 0;		// The baud rate

void CDC_InitBaud(void) {
	LineCoding[0] = 0x00;
	LineCoding[1] = 0xE1;
	LineCoding[2] = 0x00;
	LineCoding[3] = 0x00;
	LineCoding[4] = 0x00;
	LineCoding[5] = 0x00;
	LineCoding[6] = 0x08;
}

void CDC_SetBaud(void) {
	U32_XLittle(&CDC_Baud, LineCoding);

	//*((uint8_t *)&CDC_Baud) = LineCoding[0];
	//*((uint8_t *)&CDC_Baud+1) = LineCoding[1];
	//*((uint8_t *)&CDC_Baud+2) = LineCoding[2];
	//*((uint8_t *)&CDC_Baud+3) = LineCoding[3];

	if(CDC_Baud > 999999)
		CDC_Baud = 57600;
}

void USB_EP1_IN(void) {
	UEP1_T_LEN = 0;
	UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;
}

void USB_EP2_IN(void) {
	UEP2_T_LEN = 0;
	if (CDC_Tx_Full) {
		// Send a zero-length-packet(ZLP) to end this transfer
		UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;	// ACK next IN transfer
		CDC_Tx_Full = 0;
		// CDC_Tx_Busy remains set until the next ZLP sent to the host
	} else {
		UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;
		CDC_Tx_Busy = 0;
	}
}

void USB_EP2_OUT(void) {
	if (!U_TOG_OK )
		return;

	CDC_Rx_Pending = USB_RX_LEN;
	CDC_Rx_CurPos = 0;				// Reset Rx pointer
	// Reject packets by replying NAK, until uart_poll() finishes its job, then it informs the USB peripheral to accept incoming packets
	UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_NAK;
}

void CDC_PutChar(uint8_t tdata) {
	// Add new data to CDC_PutCharBuf
	CDC_PutCharBuf[CDC_PutCharBuf_Last++] = tdata;
	if(CDC_PutCharBuf_Last >= CDC_PUTCHARBUF_LEN) {
		// Rotate the tail to the beginning of the buffer
		CDC_PutCharBuf_Last = 0;
	}

	if (CDC_PutCharBuf_Last == CDC_PutCharBuf_First) {
		// Buffer is full
		CDC_Tx_Full = 1;

		while(CDC_Tx_Full)	// Wait until the buffer has vacancy
			CDC_USB_Poll();
	}
}

void CDC_Puts(char *str) {
	while(*str)
		CDC_PutChar(*(str++));
}

// Handles CDC_PutCharBuf and IN transfer
void CDC_USB_Poll() {
	static uint8_t usb_frame_count = 0;
	uint8_t usb_tx_len;

	if(UsbConfig) {
		if(usb_frame_count++ > 100) {
			usb_frame_count = 0;

			if(!CDC_Tx_Busy) {
				if(CDC_PutCharBuf_First == CDC_PutCharBuf_Last) {
					if (CDC_Tx_Full) { // Buffer is full
						CDC_Tx_Busy = 1;

						// length (the first byte to send, the end of the buffer)
						usb_tx_len = CDC_PUTCHARBUF_LEN - CDC_PutCharBuf_First;
						memcpy(EP2_TX_BUF, &CDC_PutCharBuf[CDC_PutCharBuf_First], usb_tx_len);

						// length (the first byte in the buffer, the last byte to send), if any
						if (CDC_PutCharBuf_Last > 0)
							memcpy(&EP2_TX_BUF[usb_tx_len], CDC_PutCharBuf, CDC_PutCharBuf_Last);

						// Send the entire buffer
						UEP2_T_LEN = CDC_PUTCHARBUF_LEN;
						UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;	// ACK next IN transfer

						// A 64-byte packet is going to be sent, according to USB specification, USB uses a less-than-max-length packet to demarcate an end-of-transfer
						// As a result, we need to send a zero-length-packet.
						return;
					}

					// Otherwise buffer is empty, nothing to send
					return;
				} else {
					CDC_Tx_Busy = 1;

					// CDC_PutChar() is the only way to insert into CDC_PutCharBuf, it detects buffer overflow and notify the CDC_USB_Poll().
					// So in this condition the buffer can not be full, so we don't have to send a zero-length-packet after this.

					if(CDC_PutCharBuf_First > CDC_PutCharBuf_Last) { // Rollback
						// length (the first byte to send, the end of the buffer)
						usb_tx_len = CDC_PUTCHARBUF_LEN - CDC_PutCharBuf_First;
						memcpy(EP2_TX_BUF, &CDC_PutCharBuf[CDC_PutCharBuf_First], usb_tx_len);

						// length (the first byte in the buffer, the last byte to send), if any
						if (CDC_PutCharBuf_Last > 0) {
							memcpy(&EP2_TX_BUF[usb_tx_len], CDC_PutCharBuf, CDC_PutCharBuf_Last);
							usb_tx_len += CDC_PutCharBuf_Last;
						}

						UEP2_T_LEN = usb_tx_len;
					} else {
						usb_tx_len = CDC_PutCharBuf_Last - CDC_PutCharBuf_First;
						memcpy(EP2_TX_BUF, &CDC_PutCharBuf[CDC_PutCharBuf_First], usb_tx_len);

						UEP2_T_LEN = usb_tx_len;
					}

					CDC_PutCharBuf_First += usb_tx_len;
					if(CDC_PutCharBuf_First>=CDC_PUTCHARBUF_LEN)
						CDC_PutCharBuf_First -= CDC_PUTCHARBUF_LEN;

					// ACK next IN transfer
					UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;
				}
			}
		}

	}
}


void CDC_UART_Poll() {
	uint8_t cur_byte;
	static uint8_t former_data = 0;		// Previous byte
	static uint8_t cdc_rx_state = 0;	// Rx processing state machine

	/*
	 *	I2C Tx:
	 *		bit7: If set, no stop condition will be generated at the end of current transfer
	 *		rest bits: Unused
	 *	I2C Rx:
	 *		Inapplicable
	 *	SPI Tx, Rx:
	 *		bit7: If set, CS remains low after this transfer, otherwise CS becomes high after transfer
	 *		rest bits: Unused
	 */
	static uint8_t dontstop = 0;
	static uint8_t frame_len = 0;

	// Tx only
	static uint8_t frame_sent = 0;

	// I2C only
	static uint8_t i2c_error_no = 0;

	// If there are data pending
	if(CDC_Rx_Pending) {
		cur_byte = EP2_RX_BUF[CDC_Rx_CurPos++];

		if(cdc_rx_state == CDC_STATE_IDLE) {
			if(cur_byte == 'Q')	{
				CDC_Puts(SI5351_ReferenceClock); /* 26MHz Crystal */
				CDC_Puts("\r\n");
			} else if(cur_byte == 'V') { /* Version */
				CDC_Puts(Device_Version); /* Device version */
				CDC_Puts("\r\n");
			} else if(cur_byte == 'E') {
				JumpToBootloader();
			} else if(cur_byte == 'B') {
				CDC_PutChar(CDC_Baud / 100000 + '0');
				CDC_PutChar(CDC_Baud % 100000 / 10000 + '0');
				CDC_PutChar(CDC_Baud % 10000 / 1000 + '0');
				CDC_PutChar(CDC_Baud % 1000 / 100 + '0');
				CDC_PutChar(CDC_Baud % 100 / 10 + '0');
				CDC_PutChar(CDC_Baud % 10 / 1 + '0');
				CDC_Puts("\r\n");
			}

			// I2C
			else if(cur_byte == 'T' && former_data != 'A') { /* BAN AT commands */
				// Transmit I2C Data: T: <LEN>, 16bytes data, performing S, <AR>, <DAT>, E
				frame_sent = 0;
				cdc_rx_state = CDC_STATE_I2C_TXSTART;
				i2c_error_no = 0;
				frame_len = 0;
			} else if(cur_byte == 'R') {
				// Recieve I2C Data: R<AR><LEN>
				frame_len = 0;
				cdc_rx_state = CDC_STATE_I2C_RXSTART;
			}

			// SPI
			else if(cur_byte == 'S') {
				frame_len = 0;
				cdc_rx_state = CDC_STATE_SPI_TXSTART;
			} else if (cur_byte == 'G') {
				frame_len = 0;
				cdc_rx_state = CDC_STATE_SPI_RXING;
			} else if (cur_byte == 'X') {
				frame_len = 128;
				while(frame_len--) {
					CDC_PutChar(frame_len);
				}
			}

			else if(cur_byte == 'T' && former_data == 'A') {
				/* BAN AT commands */
				CDC_Puts("OK\r\n");
			} else if(cur_byte == 'A') {

			} else {
				CDC_Puts("NOT SUPPORTED\r\n");
			}

		} // if(cdc_rx_state == CDC_STATE_IDLE)

		// I2C
		else if(cdc_rx_state == CDC_STATE_I2C_TXSTART) { // 54	03	C0	02	53
			frame_len = cur_byte & 0x3F;
			dontstop = cur_byte & CDC_FLAG_NOSTOP;

			I2C_Send_Start();
			cdc_rx_state = CDC_STATE_I2C_TXING;
		} else if(cdc_rx_state == CDC_STATE_I2C_TXING) {
			if(i2c_error_no == 0)
			{
				I2C_Buf = cur_byte;
				I2C_WriteByte();
				if(I2C_Buf) { // Received NAK
					I2C_Send_Stop();
					i2c_error_no = frame_sent + 1;
				}
			}
			frame_sent ++;

			if(frame_len == frame_sent)
			{
				if(i2c_error_no == 0)
				{
					CDC_Puts("OK\r\n");
					if(dontstop == 0)
						I2C_Send_Stop();
				}
				else
				{
					CDC_PutChar('F');	// Transmission failed
					CDC_PutChar(i2c_error_no / 10 + '0');
					CDC_PutChar(i2c_error_no % 10 + '0');
					CDC_Puts("\r\n");
				}

				frame_len = 0;
				frame_sent = 0;
				cdc_rx_state = CDC_STATE_IDLE;
				i2c_error_no = 0;
			}
		} else if(cdc_rx_state == CDC_STATE_I2C_RXSTART) {
			I2C_Send_Start();
			I2C_Buf = cur_byte;
			I2C_WriteByte();
			if(I2C_Buf) { // Received NAK
				CDC_Puts("FAIL\r\n");
				I2C_Send_Stop();
				cdc_rx_state = CDC_STATE_IDLE;
			}
			cdc_rx_state = CDC_STATE_I2C_RXING;
		} else if(cdc_rx_state == CDC_STATE_I2C_RXING) {
			frame_len = cur_byte & 0x3f;
			frame_len--;

			while (frame_len--) {
				I2C_ReadByte();
				CDC_PutChar(I2C_Buf);
				I2C_Send_ACK();
			}
			I2C_ReadByte();
			CDC_PutChar(I2C_Buf);
			I2C_Send_NACK();

			I2C_Send_Stop();
			cdc_rx_state = CDC_STATE_IDLE;
		}

		// SPI
		else if (cdc_rx_state == CDC_STATE_SPI_TXSTART) {
			frame_len = cur_byte & 0x3F;
			dontstop = cur_byte & CDC_FLAG_NOSTOP;

			// An SPI transfer starts with CS transit from H to L
			// If the last transfer is not completed, CS is L
			// So we always set CS to L here
			SPI_SetCS(0);

			// If frame_len == 0, this command will only set CS to L
			if (frame_len == 0){
				cdc_rx_state = CDC_STATE_IDLE;
			} else {
				cdc_rx_state = CDC_STATE_SPI_TXING;
			}
		} else if (cdc_rx_state == CDC_STATE_SPI_TXING) {
			SPI_MasterData(cur_byte);

			frame_sent++;
			if(frame_len == frame_sent) {
				if (!dontstop) {
					// Set CS to H, terminate this transfer
					SPI_SetCS(1);
				}

				frame_len = 0;
				frame_sent = 0;
				cdc_rx_state = CDC_STATE_IDLE;

				CDC_Puts("OK\r\n");
			}
		} else if (cdc_rx_state == CDC_STATE_SPI_RXING) {
			frame_len = cur_byte & 0x3F;
			dontstop = cur_byte & CDC_FLAG_NOSTOP;

			// If frame_len == 0, this command will only set CS to H
			if (frame_len > 0) {
				// SPI Rx must follow a Tx or Rx, therefore at this moment, normally CS should be L
				while(frame_len--) {
					CDC_PutChar(SPI_MasterData(0xFF));
				}

				if (!dontstop){
					// Set CS to H, terminate this transfer
					SPI_SetCS(1);
				}
			} else {
				SPI_SetCS(1);
			}

			cdc_rx_state = CDC_STATE_IDLE;
		}

		former_data = cur_byte;

		CDC_Rx_Pending--;

		if(CDC_Rx_Pending == 0)
			UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_ACK;
	} // if(CDC_PendingRxByte)
}

// AT2402
// Read first 5 byte: 54 82 A0 00 52 A1 05
// Write first 5 byte: 54 07 A0 00 01 02 03 04 05


// 25Q64 (ID=EF 40 17)
// Read ID: 53 81 9F 47 03
