#ifndef __USB_DESC_H
#define __USB_DESC_H

#include "ch554_platform.h"

#define USB_INTERFACES 2

// Common USB Descriptors
#define USB_DESCSIZE_DEVICE 18		// Constant, DO NOT change
#define USB_DESCSIZE_CONFIG_H 0
#define USB_DESCSIZE_CONFIG_L 75	// Actual size of your CfgDesc, set according to your configuration
// Device Descriptor
extern code const uint8_t DevDesc[];
// Configuration Descriptor, Interface Descriptors, Endpoint Descriptors and ...
extern code const uint8_t CfgDesc[];
// String Descriptors
#define USB_STRINGDESC_COUNT 4 		// Number of String Descriptors available
extern code const uint8_t* StringDescs[];

#endif /* __USB_DESC_H */
