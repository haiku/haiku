/*
 * Copyright 2011, Haiku Inc. All Rights Reserved.
 * Copyright 2009, Ithamar Adema, <ithamar.adema@team-embedded.nl>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _USB_VIDEO_H
#define _USB_VIDEO_H


#define VC_CONTROL_UNDEFINED		0x0
#define VC_VIDEO_POWER_MODE_CONTROL	0x1
#define VC_REQUEST_ERROR_CODE_CONTROL	0x2

typedef struct usbvc_class_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
} _PACKED usbvc_class_descriptor;

typedef uint8 usbvc_guid[16];

struct usbvc_format_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	formatIndex;
	uint8	numFrameDescriptors;
	union {
		struct {
			usbvc_guid format;
			uint8	bytesPerPixel;
			uint8	defaultFrameIndex;
			uint8	aspectRatioX;
			uint8	aspectRatioY;
			uint8	interlaceFlags;
			uint8	copyProtect;
		} uncompressed;
		struct {
			uint8	flags;
			uint8	defaultFrameIndex;
			uint8	aspectRatioX;
			uint8	aspectRatioY;
			uint8	interlaceFlags;
			uint8	copyProtect;
		} mjpeg;
	};
} _PACKED;

struct usbvc_interface_header_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint16	version;
	uint16	totalLength;
	uint32	clockFrequency;
	uint8	numInterfacesNumbers;
	uint8	interfaceNumbers[0];
} _PACKED;

struct usbvc_input_terminal_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	terminalID;
	uint16	terminalType;
	uint8	associatedTerminal;
	uint8	terminal;
} _PACKED;


#endif /* _USB_VIDEO_H */
