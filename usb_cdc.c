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

extern uint8_t UsbConfig;
idata uint8_t  Receive_Uart_Buf[UART_REV_LEN];
idata volatile uint8_t Uart_Input_Point = 0;   //循环缓冲区写入指针，总线复位需要初始化为0
idata volatile uint8_t Uart_Output_Point = 0;  //循环缓冲区取出指针，总线复位需要初始化为0
idata volatile uint8_t UartByteCount = 0;	  //当前缓冲区剩余待取字节数
idata volatile uint8_t CDC_PendingRxByte = 0;	  //代表USB端点接收到的数据
idata volatile uint8_t CDC_CurRxPos = 0;	//取数据指针
idata volatile uint8_t UpPoint2_Busy  = 0;   //上传端点是否忙标志
uint32_t CDC_Baud = 0;


void USB_EP1_IN(void) {
	UEP1_T_LEN = 0;
	UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;
}

void USB_EP2_IN(void) {
	UEP2_T_LEN = 0;
	UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;
	UpPoint2_Busy = 0;
}

void USB_EP2_OUT(void) {
	if (!U_TOG_OK )
		return;

	CDC_PendingRxByte = USB_RX_LEN;
	CDC_CurRxPos = 0;				// Reset Rx pointer
	// Reject packets by replying NAK, until uart_poll() finishes its job, then it informs the USB peripheral to accept incoming packets
	UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_NAK;
}

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

void CDC_PutChar(uint8_t tdata) {
	Receive_Uart_Buf[Uart_Input_Point++] = tdata;
	UartByteCount++;					//当前缓冲区剩余待取字节数
	if(Uart_Input_Point>=UART_REV_LEN)
	{
		Uart_Input_Point = 0;
	}
}

void CDC_Puts(char *str) {
	while(*str)
		CDC_PutChar(*(str++));
}

void usb_poll()
{
	uint8_t length;
	static uint8_t Uart_Timeout = 0;
	if(UsbConfig)
	{
		if(UartByteCount)
			Uart_Timeout++;
		if(!UpPoint2_Busy)   //端点不繁忙（空闲后的第一包数据，只用作触发上传）
		{
			length = UartByteCount;
			if(length>0)
			{
				if(length>39 || Uart_Timeout>100)
				{
					Uart_Timeout = 0;
					if(Uart_Output_Point+length>UART_REV_LEN)
						length = UART_REV_LEN-Uart_Output_Point;
					UartByteCount -= length;
					//写上传端点
					memcpy(EP2_TX_BUF, &Receive_Uart_Buf[Uart_Output_Point],length);
					Uart_Output_Point+=length;
					if(Uart_Output_Point>=UART_REV_LEN)
						Uart_Output_Point = 0;
					UEP2_T_LEN = length;													//预使用发送长度一定要清空
					UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;			//应答ACK
					UpPoint2_Busy = 1;
				}
			}
		}
	}
}


void uart_poll() {
	uint8_t cur_byte;
	static uint8_t former_data = 0; // Previous byte
	static uint8_t cdc_rx_state = 0;

	/*
	 *	I2C Tx:
	 *		bit7: If set, no stop condition will be generated at the end of current transfer
	 *		rest bits: Unused
	 *	I2C Rx:
	 *		Inapplicable
	 *	SPI Tx, Rx:
	 *		bit7: If set, CS remains low after this transfer
	 *		rest bits: Unused
	 */
	static uint8_t dontstop = 0;
	static uint8_t frame_len = 0;

	// Tx only
	static uint8_t frame_sent = 0;

	// I2C only
	static uint8_t i2c_error_no = 0;

	// If there are data pending
	if(CDC_PendingRxByte) {
		cur_byte = EP2_RX_BUF[CDC_CurRxPos++];

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

			cdc_rx_state = CDC_STATE_SPI_TXING;
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
			}
		} else if (cdc_rx_state == CDC_STATE_SPI_RXING) {
			frame_len = cur_byte & 0x3F;
			dontstop = cur_byte & CDC_FLAG_NOSTOP;

			// SPI Rx must follow a Tx or Rx, therefore at this moment, normally CS should be L
			while(frame_len--) {
				CDC_PutChar(SPI_MasterData(0xFF));
			}

			if (!dontstop){
				// Set CS to H, terminate this transfer
				SPI_SetCS(1);
			}

			cdc_rx_state = CDC_STATE_IDLE;
		}

		former_data = cur_byte;

		CDC_PendingRxByte--;

		if(CDC_PendingRxByte == 0)
			UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_ACK;
	} // if(CDC_PendingRxByte)
}

// AT2402
// Read first 5 byte: 54 82 A0 00 52 A1 05
// Write first 5 byte: 54 07 A0 00 01 02 03 04 05


// 25Q64 (ID=EF 40 17)
// Read ID: 53 81 9F 47 03
