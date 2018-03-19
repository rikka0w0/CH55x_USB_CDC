#include "usb_desc.h"

#include "ch554_platform.h"

// Device Descriptor
code const uint8_t DevDesc[] = {	
	USB_DESCSIZE_DEVICE,	// Total length
	0x01,				// Type: Device Descriptor
	0x10, 0x01,	// USB Spec., 0x0110 -> USB 1.1
	0x02,				// Class code, 00 - each interface defines its own class
	0x00,				// Device Subclass
	0x00,				// Device Protocol
	8,					// Max packet size
	0x86, 0x1a,	// VID	0x413d
	0x22, 0x57,	// PID	0x2107
	0x00, 0x01,	// Device release number in BCD
	0x04,				// Manufactor, index of string descriptor
	0x03,				// Product string descriptor ID
	0x00,				// Serial number (String descriptor ID) 
	0x01				// Number of available configurations
};


// Configuration Descriptor and Interface Descriptor
code const uint8_t CfgDesc[] =
{
	// Configuration Descriptor
	9,					// Length of the descriptor
	0x02,				// Type: Configuration Descriptor
	// Total length of this and following descriptors
	USB_DESCSIZE_CONFIG_L, USB_DESCSIZE_CONFIG_H,
	USB_INTERFACES,		// Number of interfaces
	0x01, 			// Value used to select this configuration
	0x00,				// Index of corresponding string descriptor
	0x80,				// Attributes, D7 must be 1, D6 Self-powered, D5 Remote Wakeup, D4-D0=0
	0x32,				// Max current drawn by device, in units of 2mA


	// Interface descriptor (CDC)
	9,					// Length of the descriptor
	0x04,				// Type: Interface Descriptor
	0x00,				// Interface ID
	0x00,				// Alternate setting
	0x01,				// Number of Endpoints
	0x02,				// Interface class code
	0x02,				// Subclass code - Abstract Control Model
	0x01,				// Protocol code - AT Command V.250 protocol
	0x00,				// Index of corresponding string descriptor (On Windows, it is called "Bus reported device description")

	// Header Functional descriptor (CDC)
	0x05,				// Length of the descriptor
	0x24,				// bDescriptortype, CS_INTERFACE
	0x00,				// bDescriptorsubtype, HEADER
	0x10,0x01,			// bcdCDC

	//
	0x05,0x24,0x01,0x00,0x00,								 //管理描述符(没有数据类接口) 03 01
	0x04,0x24,0x02,0x02,									  //支持Set_Line_Coding、Set_Control_Line_State、Get_Line_Coding、Serial_State
	0x05,0x24,0x06,0x00,0x01,								 //编号为0的CDC接口;编号1的数据类接口

	// EndPoint descriptor (CDC Upload, Interrupt)
	7,					// Length of the descriptor
	0x05,				// Type: Endpoint Descriptor
	0x81,				// Endpoint: D7: 0-Out 1-In, D6-D4=0, D3-D0 Endpoint number
	0x03,				// Attributes:
						// D1:0 Transfer type: 00 = Control 01 = Isochronous 10 = Bulk 11 = Interrupt
						// 			The following only apply to isochronous endpoints. Else set to 0.
						// D3:2 Synchronisation Type: 00 = No Synchronisation 01 = Asynchronous 10 = Adaptive 11 = Synchronous
						// D5:4	Usage Type: 00 = Data endpoint 01 = Feedback endpoint 10 = Implicit feedback Data endpoint 11 = Reserved
						// D7:6 = 0
	0x10, 0x00,			// Maximum packet size can be handled
	0x40,				// Interval for polling, in units of 1 ms for low/full speed

	// Interface descriptor (CDC)
	9,					// Length of the descriptor
	0x04,				// Type: Interface Descriptor
	0x01,				// Interface ID
	0x00,				// Alternate setting
	0x02,				// Number of Endpoints
	0x0a,				// Interface class code
	0x00,				// Subclass code
	0x00,				// Protocol code
	0x00,				// Index of corresponding string descriptor (On Windows, it is called "Bus reported device description")

	// EndPoint descriptor (CDC Upload, Bulk)
	7,					// Length of the descriptor
	0x05,				// Type: Endpoint Descriptor
	0x02,				// Endpoint: D7: 0-Out 1-In, D6-D4=0, D3-D0 Endpoint number
	0x02,				// Attributes:
						// D1:0 Transfer type: 00 = Control 01 = Isochronous 10 = Bulk 11 = Interrupt
						// 			The following only apply to isochronous endpoints. Else set to 0.
						// D3:2 Synchronisation Type: 00 = No Synchronisation 01 = Asynchronous 10 = Adaptive 11 = Synchronous
						// D5:4	Usage Type: 00 = Data endpoint 01 = Feedback endpoint 10 = Implicit feedback Data endpoint 11 = Reserved
						// D7:6 = 0
	0x40, 0x00,			// Maximum packet size can be handled
	0x00,				// Interval for polling, in units of 1 ms for low/full speed

	// EndPoint descriptor (CDC Upload, Bulk)
	7,				// Length of the descriptor
	0x05,				// Type: Endpoint Descriptor
	0x82,				// Endpoint: D7: 0-Out 1-In, D6-D4=0, D3-D0 Endpoint number
	0x02,				// Attributes:
						// D1:0 Transfer type: 00 = Control 01 = Isochronous 10 = Bulk 11 = Interrupt
						// 			The following only apply to isochronous endpoints. Else set to 0.
						// D3:2 Synchronisation Type: 00 = No Synchronisation 01 = Asynchronous 10 = Adaptive 11 = Synchronous
						// D5:4	Usage Type: 00 = Data endpoint 01 = Feedback endpoint 10 = Implicit feedback Data endpoint 11 = Reserved
						// D7:6 = 0
	0x40, 0x00,			// Maximum packet size can be handled
	0x00				// Interval for polling, in units of 1 ms for low/full speed
};
