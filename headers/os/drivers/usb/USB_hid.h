#ifndef USB_HID_H
#define USB_HID_H

// (Partial) USB Class Definitions for HID Devices, version 1.11
// Reference: http://www.usb.org/developers/devclass_docs/hid1_11.pdf

#define USB_HID_DEVICE_CLASS 			0x03
#define USB_HID_CLASS_VERSION			0x0100

enum { // HID Interface Subclasses
	USB_HID_INTERFACE_NO_SUBCLASS = 0x00,	//  No Subclass
	USB_HID_INTERFACE_BOOT_SUBCLASS			//	Boot Interface Subclass
};

enum { // HID Class-Specific descriptor subtypes
	USB_HID_DESCRIPTOR_HID						= 0x21,
	USB_HID_DESCRIPTOR_REPORT,
	USB_HID_DESCRIPTOR_PHYSICAL
};

enum { // HID Class-specific requests
	USB_REQUEST_HID_GET_REPORT		= 0x01,
	USB_REQUEST_HID_GET_IDLE,
	USB_REQUEST_HID_GET_PROTOCOL,
	
	USB_REQUEST_HID_SET_REPORT = 0x09,
	USB_REQUEST_HID_SET_IDLE,
	USB_REQUEST_HID_SET_PROTOCOL
};

typedef struct
{
	uint8	length;
	uint8	descriptor_type;
	uint16	hid_version;
	uint8	country_code;
	uint8	num_descriptors;
	struct
	{
		uint8	descriptor_type;
		uint16	descriptor_length;
	} _PACKED descriptor_info [1];
} _PACKED usb_hid_descriptor;

#endif	// USB_HID_H
