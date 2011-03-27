/*
 * Copyright 2011, Haiku Inc. All Rights Reserved.
 * Copyright 2009, Ithamar Adema, <ithamar.adema@team-embedded.nl>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _USB_VIDEO_H
#define _USB_VIDEO_H


/* Class/Subclass/Protocol */
#define CC_VIDEO				0xE
#define SC_UNDEFINED			0x0
#define SC_VIDEOCONTROL			0x1
#define SC_VIDEOSTREAMING		0x2
#define SC_VIDEO_INTERFACE_COLLECTION	0x3
#define PC_PROTOCOL_UNDEFINED		0x0

#define CS_UNDEFINED			0x20
#define CS_DEVICE				0x21
#define CS_CONFIGURATION		0x22
#define CS_STRING				0x23
#define CS_INTERFACE			0x24
#define CS_ENDPOINT				0x25

/* Video Control Class Descriptors */
#define VC_DESCRIPTOR_UNDEFINED	0x0
#define VC_HEADER				0x1
#define VC_INPUT_TERMINAL		0x2
#define VC_OUTPUT_TERMINAL		0x3
#define VC_SELECTOR_UNIT		0x4
#define VC_PROCESSING_UNIT		0x5
#define VC_EXTENSION_UNIT		0x6

/* Video Streaming Class Descriptors */
#define VS_UNDEFINED			0x00
#define VS_INPUT_HEADER			0x01
#define VS_OUTPUT_HEADER		0x02
#define VS_STILL_IMAGE_FRAME	0x03
#define VS_FORMAT_UNCOMPRESSED	0x04
#define VS_FRAME_UNCOMPRESSED	0x05
#define VS_FORMAT_MJPEG			0x06
#define VS_FRAME_MJPEG			0x07
#define VS_FORMAT_MPEG2TS		0x0a
#define VS_FORMAT_DV			0x0c
#define VS_COLORFORMAT			0x0d
#define VS_FORMAT_FRAME_BASED	0x10
#define VS_FRAME_FRAME_BASED	0x11
#define VS_FORMAT_STREAM_BASED	0x12

#define EP_UNDEFINED			0x0
#define EP_GENERAL				0x1
#define EP_ENDPOINT				0x2
#define EP_INTERRUPT			0x3

#define RC_UNDEFINED			0x00
#define SET_CUR					0x01
#define GET_CUR					0x81
#define GET_MIN					0x82
#define GET_MAX					0x83
#define GET_RES					0x84
#define GET_LEN					0x85
#define GET_INFO				0x86
#define GET_DEF					0x87

#define VC_CONTROL_UNDEFINED		0x0
#define VC_VIDEO_POWER_MODE_CONTROL	0x1
#define VC_REQUEST_ERROR_CODE_CONTROL	0x2

#define TE_CONTROL_UNDEFINED		0x0

#define SU_CONTROL_UNDEFINED		0x0
#define SU_INPUT_SELECT_CONTROL		0x1

#define CT_CONTROL_UNDEFINED		0x0
#define CT_SCANNING_MODE_CONTROL	0x1
#define CT_AE_MODE_CONTROL			0x2
#define CT_AE_PRIORITY_CONTROL		0x3
#define CT_EXPOSURE_TIME_ABSOLUTE_CONTROL	0x4
#define CT_EXPOSURE_TIME_RELATIVE_CONTROL	0x5
#define CT_FOCUS_ABSOLUTE_CONTROL	0x6
#define CT_FOCUS_RELATIVE_CONTROL	0x7
#define CT_FOCUS_AUTO_CONTROL		0x8
#define CT_IRIS_ABSOLUTE_CONTROL	0x9
#define CT_IRIS_RELATIVE_CONTROL	0xa
#define CT_ZOOM_ABSOLUTE_CONTROL	0xb
#define CT_ZOOM_RELATIVE_CONTROL	0xc
#define CT_PANTILT_ABSOLUTE_CONTROL	0xd
#define CT_PANTILT_RELATIVE_CONTROL	0xe
#define CT_ROLL_ABSOLUTE_CONTROL	0xf
#define CT_ROLL_RELATIVE_CONTROL	0x10
#define CT_PRIVACY_CONTROL			0x11

#define PU_CONTROL_UNDEFINED		0x0
#define PU_BACKLIGHT_COMPENSATION_CONTROL 0x1
#define PU_BRIGHTNESS_CONTROL		0x2
#define PU_CONTRAST_CONTROL			0x3
#define PU_GAIN_CONTROL				0x4
#define PU_POWER_LINE_FREQUENCY_CONTROL	0x5
#define PU_HUE_CONTROL				0x6
#define PU_SATURATION_CONTROL		0x7
#define PU_SHARPNESS_CONTROL		0x8
#define PU_GAMMA_CONTROL			0x9
#define PU_WHITE_BALANCE_TEMPERATURE_CONTROL	0xa
#define PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL	0xb
#define PU_WHITE_BALANCE_COMPONENT_CONTROL	0xc
#define PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL	0xd
#define PU_DIGITAL_MULTIPLIER_CONTROL	0xe
#define PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL	0xf
#define PU_HUE_AUTO_CONTROL			0x10
#define PU_ANALOG_VIDEO_STANDARD_CONTROL	0x11
#define PU_ANALOG_LOCK_STATUS_CONTROL	0x12

#define XU_CONTROL_UNDEFINED		0x0

#define VS_CONTROL_UNDEFINED		0x0
#define VS_PROBE_CONTROL			0x1
#define VS_COMMIT_CONTROL			0x2
#define VS_STILL_PROBE_CONTROL		0x3
#define VS_STILL_COMMIT_CONTROL		0x4
#define VS_STILL_IMAGE_TRIGGER_CONTROL	0x5
#define VS_STREAM_ERROR_CODE_CONTROL	0x6
#define VS_GENERATE_KEY_FRAME_CONTROL	0x7
#define VS_UPDATE_FRAME_SEGMENT_CONTROL	0x8
#define VS_SYNCH_DELAY_CONTROL		0x9

typedef struct usbvc_class_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;	
} _PACKED usbvc_class_descriptor;

struct usbvc_input_header_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	numFormats;
	uint16	totalLength;
	uint8	endpointAddress;
	uint8	info;
	uint8	terminalLink;
	uint8	stillCaptureMethod;
	uint8	triggerSupport;
	uint8	triggerUsage;
	uint8	controlSize;
	uint8	controls[0];
} _PACKED;

struct usbvc_output_header_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	numFormats;
	uint16	totalLength;
	uint8	endpointAddress;
	uint8	terminalLink;
	uint8	controlSize;
	uint8	controls[0];
} _PACKED;

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

struct usbvc_frame_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	frameIndex;
	uint8	capabilities;
	uint16	width;
	uint16	height;
	uint32	minBitRate;
	uint32	maxBitRate;
	uint32	maxVideoFrameBufferSize;
	uint32	defaultFrameInterval;
	uint8	frameIntervalType;
	union {
		struct {
			uint32	minFrameInterval;
			uint32	maxFrameInterval;
			uint32	frameIntervalStep;
		} continuous;
		uint32	discreteFrameIntervals[0];
	};
} _PACKED;

typedef struct {
	uint16	width;
	uint16	height;
} _PACKED usbvc_image_size_pattern;

struct usbvc_still_image_frame_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	endpointAddress;
	uint8	numImageSizePatterns;
	usbvc_image_size_pattern imageSizePatterns[0];
	uint8 NumCompressionPatterns() const { return *(CompressionPatterns() - 1); }
	const uint8* CompressionPatterns() const {
		return ((const uint8*)imageSizePatterns + sizeof(usbvc_image_size_pattern)
			* numImageSizePatterns + sizeof(uint8));
	} 
} _PACKED;

struct usbvc_color_matching_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	colorPrimaries;
	uint8	transferCharacteristics;
	uint8	matrixCoefficients;
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

struct usbvc_output_terminal_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	terminalID;
	uint16	terminalType;
	uint8	associatedTerminal;
	uint8	sourceID;
	uint8	terminal;
} _PACKED;

struct usbvc_camera_terminal_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	terminalID;
	uint16	terminalType;
	uint8	associatedTerminal;
	uint8	terminal;
	uint16	objectiveFocalLengthMin;
	uint16	objectiveFocalLengthMax;
	uint16	ocularFocalLength;
	uint8	controlSize;
	uint8	controls[0];
} _PACKED;

struct usbvc_selector_unit_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	unitID;
	uint8	numInputPins;
	uint8	sourceID[0];
	uint8	Selector() const { return sourceID[numInputPins]; }
} _PACKED;

struct usbvc_processing_unit_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	unitID;
	uint8	sourceID;
	uint16	maxMultiplier;
	uint8	controlSize;
	uint8	controls[0];
	uint8	Processing() const { return controls[controlSize]; }
	uint8	VideoStandards() const { return controls[controlSize+1]; }
} _PACKED;

struct usbvc_extension_unit_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	unitID;
	usbvc_guid	guidExtensionCode;
	uint8	numControls;
	uint8	numInputPins;
	uint8	sourceID[0];
	uint8	ControlSize() const { return sourceID[numInputPins]; }
	const uint8*	Controls() const { return &sourceID[numInputPins+1]; }
	uint8	Extension() const
		{	return sourceID[numInputPins + ControlSize() + 1]; }
} _PACKED;

struct usbvc_probecommit {
	uint16	hint;
	uint8	formatIndex;
	uint8	frameIndex;
	uint32	frameInterval;
	uint16	keyFrameRate;
	uint16	pFrameRate;
	uint16	compQuality;
	uint16	compWindowSize;
	uint16	delay;
	uint32	maxVideoFrameSize;
	uint32	maxPayloadTransferSize;
	uint32	clockFrequency;
	uint8	framingInfo;
	uint8	preferredVersion;
	uint8	minVersion;
	uint8	maxVersion;
	void	SetFrameInterval(uint32 interval)
		{ frameInterval = interval; }
} _PACKED;


#endif /* _USB_VIDEO_H */
