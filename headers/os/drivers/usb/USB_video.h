/*
 * Copyright 2016, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under terms of the MIT license.
 */


#ifndef USB_VIDEO_H
#define USB_VIDEO_H


// Based on specification for UVC version 1.5.


#include <SupportDefs.h>


#define USB_VIDEO_DEVICE_CLASS 			0x0E

enum { // Video Interface Subclasses
	USB_VIDEO_INTERFACE_UNDEFINED_SUBCLASS		= 0x00,
	USB_VIDEO_INTERFACE_VIDEOCONTROL_SUBCLASS	= 0x01,
	USB_VIDEO_INTERFACE_VIDEOSTREAMING_SUBCLASS,
	USB_VIDEO_INTERFACE_COLLECTION_SUBCLASS,
};


enum { // Video Interface Protocol Codes
	USB_VIDEO_PROTOCOL_UNDEFINED				= 0x00,
	USB_VIDEO_PROTOCOL_15						= 0x01,
};


enum { // Video Interface Class-Specific Descriptor Types
	USB_VIDEO_CS_UNDEFINED						= 0x20,
	USB_VIDEO_CS_DEVICE							= 0x21,
	USB_VIDEO_CS_CONFIGURATION					= 0x22,
	USB_VIDEO_CS_STRING							= 0x23,
	USB_VIDEO_CS_INTERFACE						= 0x24,
	USB_VIDEO_CS_ENDPOINT						= 0x25,
};


enum { // Video Class-Specific VideoControl Interface descriptor subtypes
	USB_VIDEO_VC_DESCRIPTOR_UNDEFINED			= 0x00,
	USB_VIDEO_VC_HEADER							= 0x01,
	USB_VIDEO_VC_INPUT_TERMINAL					= 0x02,
	USB_VIDEO_VC_OUTPUT_TERMINAL				= 0x03,
	USB_VIDEO_VC_SELECTOR_UNIT					= 0x04,
	USB_VIDEO_VC_PROCESSING_UNIT				= 0x05,
	USB_VIDEO_VC_EXTENSION_UNIT					= 0x06,
	USB_VIDEO_VC_ENCODING_UNIT					= 0x07,
};


enum { // Video Class-Specific VideoStreaming Interface descriptor subtypes
	USB_VIDEO_VS_UNDEFINED						= 0x00,
	USB_VIDEO_VS_INPUT_HEADER					= 0x01,
	USB_VIDEO_VS_OUTPUT_HEADER					= 0x02,
	USB_VIDEO_VS_STILL_IMAGE_FRAME				= 0x03,
	USB_VIDEO_VS_FORMAT_UNCOMPRESSED			= 0x04,
	USB_VIDEO_VS_FRAME_UNCOMPRESSED				= 0x05,
	USB_VIDEO_VS_FORMAT_MJPEG					= 0x06,
	USB_VIDEO_VS_FRAME_MJPEG					= 0x07,
	USB_VIDEO_VS_FORMAT_MPEG2TS					= 0x0a,
	USB_VIDEO_VS_FORMAT_DV						= 0x0c,
	USB_VIDEO_VS_COLORFORMAT					= 0x0d,
	USB_VIDEO_VS_FORMAT_FRAME_BASED				= 0x10,
	USB_VIDEO_VS_FRAME_FRAME_BASED				= 0x11,
	USB_VIDEO_VS_FORMAT_STREAM_BASED			= 0x12,
	USB_VIDEO_VS_FORMAT_H264					= 0x13,
	USB_VIDEO_VS_FRAME_H264						= 0x14,
	USB_VIDEO_VS_FORMAT_H264_SIMULCAST			= 0x15,
	USB_VIDEO_VS_FORMAT_VP8						= 0x16,
	USB_VIDEO_VS_FRAME_VP8						= 0x17,
	USB_VIDEO_VS_FORMAT_VP8_SIMULCAST			= 0x18,
};


enum {
	// USB Terminal Types
	USB_VIDEO_VENDOR_USB_IO						= 0x100,
	USB_VIDEO_STREAMING_USB_IO					= 0x101,
	// Input terminal types
	USB_VIDEO_VENDOR_IN							= 0x200,
	USB_VIDEO_CAMERA_IN							= 0x201,
	USB_VIDEO_MEDIA_TRANSPORT_IN				= 0x202,
	// Output terminal types
	USB_VIDEO_VENDOR_OUT						= 0x300,
	USB_VIDEO_DISPLAY_OUT						= 0x301,
	USB_VIDEO_MEDIA_TRANSPORT_OUT				= 0x302,
	// External terminal types
	USB_VIDEO_VENDOR_EXT						= 0x400,
	USB_VIDEO_COMPOSITE_EXT						= 0x401,
	USB_VIDEO_SVIDEO_EXT						= 0x402,
	USB_VIDEO_COMPONENT_EXT						= 0x403,
};


enum {
	EP_SUBTYPE_UNDEFINED						= 0x00,
	EP_SUBTYPE_GENERAL							= 0x01,
	EP_SUBTYPE_ENDPOINT							= 0x02,
	EP_SUBTYPE_INTERRUPT						= 0x03,
};


// Class Specific Video Control Interface Header
// 1.5: Table 3-3 p.48
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// USB_AUDIO_CS_INTERFACE
	uint8	descriptor_subtype; 	// USB_AUDIO_AC_HEADER
	uint16	bcd_release_no;
	uint16	total_length;
	uint32	clock_frequency;
	uint8	in_collection;
	uint8	interface_numbers[0];
} _PACKED usb_videocontrol_header_descriptor;


// Input Terminal Descriptor
// 1.5: Table 3-4, page 50
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// USB_VIDEO_CS_INTERFACE
	uint8	descriptor_subtype; 	// USB_VIDEO_VC_INPUT_TERMINAL
	uint8	terminal_id;
	uint16	terminal_type;
	uint8	assoc_terminal;
	uint8	terminal;

	union {
		struct {
			uint16	focal_length_min;
			uint16	focal_length_max;
			uint16	focal_length;
			uint8	control_size;
			uint8	controls[3];
		} _PACKED camera;
	};
} _PACKED usb_video_input_terminal_descriptor;


// Output terminal descriptor
// 1.5: Table 3-5, page 51
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// USB_VIDEO_CS_INTERFACE
	uint8	descriptor_subtype; 	// USB_VIDEO_VC_OUTPUT_TERMINAL
	uint8	terminal_id;
	uint16	terminal_type;
	uint8	assoc_terminal;
	uint8	source_id;
	uint8	terminal;
} _PACKED usb_video_output_terminal_descriptor;


// Processing unit descriptor
// 1.5: Table 3-8, page 54
typedef struct {
	uint8	length;
	uint8	descriptor_type;
	uint8	descriptor_subtype;
	uint8	unit_id;
	uint8	source_id;
	uint16	max_multiplier;
	uint8	control_size;
	uint8	controls[3];
	uint8	processing;
	uint8	video_standards;
} _PACKED usb_video_processing_unit_descriptor;


struct usb_video_frame_descriptor {
	uint8	length;
	uint8	descriptor_type;
	uint8	descriptor_subtype;
	uint8	frame_index;
	uint8	capabilities;
	uint16	width;
	uint16	height;
	uint32	min_bit_rate;
	uint32	max_bit_rate;
	uint32	max_video_frame_buffer_size;
	uint32	default_frame_interval;
	uint8	frame_interval_type;
	union {
		struct {
			uint32	min_frame_interval;
			uint32	max_frame_tnterval;
			uint32	frame_interval_step;
		} continuous;
		uint32	discrete_frame_intervals[0];
	};
} _PACKED;


#endif /* !USB_VIDEO_H */
