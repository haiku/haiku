/*
 * Copyright 2016, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under terms of the MIT license.
 */


#ifndef USB_VIDEO_H
#define USB_VIDEO_H


// Based on specification for UVC version 1.5.


#include <BeBuild.h>	// for _PACKED definition
#include <lendian_bitfield.h>
#include <SupportDefs.h>


enum { // Video Interface Class Code
	USB_VIDEO_DEVICE_CLASS		= 0x0e
};


enum { // Video Interface Subclasses
	USB_VIDEO_INTERFACE_UNDEFINED_SUBCLASS		= 0x00,
	USB_VIDEO_INTERFACE_VIDEOCONTROL_SUBCLASS	= 0x01,
	USB_VIDEO_INTERFACE_VIDEOSTREAMING_SUBCLASS = 0x02,
	USB_VIDEO_INTERFACE_COLLECTION_SUBCLASS 	= 0x03,
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


enum { // Video Streaming Interface Control Selectors
	USB_VIDEO_VS_CONTROL_UNDEFINED 				= 0x00,
	USB_VIDEO_VS_PROBE_CONTROL 					= 0x01,
	USB_VIDEO_VS_COMMIT_CONTROL					= 0x02,
	USB_VIDEO_VS_STILL_PROBE_CONTROL			= 0x03,
	USB_VIDEO_VS_STILL_COMMIT_CONTROL			= 0x04,
	USB_VIDEO_VS_STILL_IMAGE_TRIGGER_CONTROL 	= 0x05,
	USB_VIDEO_VS_STREAM_ERROR_CODE_CONTROL 		= 0x06,
	USB_VIDEO_VS_GENERATE_KEY_FRAME_CONTROL		= 0x07,
	USB_VIDEO_VS_UPDATE_FRAME_SEGMENT_CONTROL	= 0x08,
	USB_VIDEO_VS_SYNCH_DELAY_CONTROL 			= 0x09
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


enum { // Video Class-Specific Endpoint Descriptor Subtypes
	EP_SUBTYPE_UNDEFINED						= 0x00,
	EP_SUBTYPE_GENERAL							= 0x01,
	EP_SUBTYPE_ENDPOINT							= 0x02,
	EP_SUBTYPE_INTERRUPT						= 0x03,
};


enum { // Terminal Control Selectors
	USB_VIDEO_TE_CONTROL_UNDEFINED 				= 0x00
};


enum { // Selector Unit Control Selectors
	USB_VIDEO_SU_CONTROL_UNDEFINED 				= 0x00,
	USB_VIDEO_SU_INPUT_SELECT_CONTROL			= 0x01
};


enum { // Video Class-Specific Request Codes
	USB_VIDEO_RC_UNDEFINED 						= 0x00,
	USB_VIDEO_RC_SET_CUR 						= 0x01,
	USB_VIDEO_RC_GET_CUR 						= 0x81,
	USB_VIDEO_RC_GET_MIN 						= 0x82,
	USB_VIDEO_RC_GET_MAX					 	= 0x83,
	USB_VIDEO_RC_GET_RES 						= 0x84,
	USB_VIDEO_RC_GET_LEN 						= 0x85,
	USB_VIDEO_RC_GET_INFO 						= 0x86,
	USB_VIDEO_RC_GET_DEF 						= 0x87
};


enum { // Camera Terminal Control Selectors
	USB_VIDEO_CT_CONTROL_UNDEFINED 				= 0x00,
	USB_VIDEO_CT_SCANNING_MODE_CONTROL 			= 0x01,
	USB_VIDEO_CT_AE_MODE_CONTROL 				= 0x02,
	USB_VIDEO_CT_AE_PRIORITY_CONTROL 			= 0x03,
	USB_VIDEO_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL = 0x04,
	USB_VIDEO_CT_EXPOSURE_TIME_RELATIVE_CONTROL = 0x05,
	USB_VIDEO_CT_FOCUS_ABSOLUTE_CONTROL 		= 0x06,
	USB_VIDEO_CT_FOCUS_RELATIVE_CONTROL 		= 0x07,
	USB_VIDEO_CT_FOCUS_AUTO_CONTROL 			= 0x08,
	USB_VIDEO_CT_IRIS_ABSOLUTE_CONTROL 			= 0x09,
	USB_VIDEO_CT_IRIS_RELATIVE_CONTROL 			= 0x0A,
	USB_VIDEO_CT_ZOOM_ABSOLUTE_CONTROL 			= 0x0B,
	USB_VIDEO_CT_ZOOM_RELATIVE_CONTROL 			= 0x0C,
	USB_VIDEO_CT_PANTILT_ABSOLUTE_CONTROL 		= 0x0D,
	USB_VIDEO_CT_PANTILT_RELATIVE_CONTROL 		= 0x0E,
	USB_VIDEO_CT_ROLL_ABSOLUTE_CONTROL 			= 0x0F,
	USB_VIDEO_CT_ROLL_RELATIVE_CONTROL 			= 0x10,
	USB_VIDEO_CT_PRIVACY_CONTROL 				= 0x11
};


enum { // Processing Unit Control Selectors
	USB_VIDEO_PU_CONTROL_UNDEFINED 							= 0x00,
	USB_VIDEO_PU_BACKLIGHT_COMPENSATION_CONTROL 			= 0x01,	
	USB_VIDEO_PU_BRIGHTNESS_CONTROL 						= 0x02,
	USB_VIDEO_PU_CONTRAST_CONTROL 							= 0x03,
	USB_VIDEO_PU_GAIN_CONTROL 								= 0x04,
	USB_VIDEO_PU_POWER_LINE_FREQUENCY_CONTROL 				= 0x05,
	USB_VIDEO_PU_HUE_CONTROL 								= 0x06,
	USB_VIDEO_PU_SATURATION_CONTROL 						= 0x07,	
	USB_VIDEO_PU_SHARPNESS_CONTROL 							= 0x08,
	USB_VIDEO_PU_GAMMA_CONTROL 								= 0x09,
	USB_VIDEO_PU_WHITE_BALANCE_TEMPERATURE_CONTROL 			= 0x0A,
	USB_VIDEO_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL 	= 0x0B,
	USB_VIDEO_PU_WHITE_BALANCE_TEMPERATURE_COMPONENT_CONTROL 	= 0x0C,
	USB_VIDEO_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL 		= 0x0D,
	USB_VIDEO_PU_DIGITAL_MULTIPLEXER_CONTROL 				= 0x0E,
	USB_VIDEO_PU_DIGITAL_MULTIPLEXER_LIMIT_CONTROL 			= 0x0F,
	USB_VIDEO_PU_HUE_AUTO_CONTROL 							= 0x10,
	USB_VIDEO_PU_ANALOG_VIDEO_STANDARD_CONTROL				= 0x11,
	USB_VIDEO_PU_ANALOG_LOCK_STATUS_CONTROL 				= 0x12
};


enum { // Extension Unit Control Selectors
	USB_VIDEO_XU_CONTROL_UNDEFINED = 0x00
};


typedef struct {
	/* The orignator bitfield of the status_type struct can have the
	following values: 0 = reserved, 1 = VideoControl interface,
	2 = VideoStreaming interface. */
	struct status_type {
		B_LBITFIELD8_2 (
				originator: 4,
				reserved: 4
			     );
	} status_type;
	/* The second common status_packet_format field orignator includes ID of
	 * the Terminal, Unit or Interface that reports the interrupt. */
	uint8 originator;
	union {
		struct videocontrol_interface_as_orignator {
			/* If the orginator bitfield value is 1 (VideoControl interface)
			 * then third field of the status_packet_format is event field that
			 * includes value 0x00 for control change. Other values
			 * (0x01 - 0xFF) are reserved. */
			uint8 event;
			/* The fourth field selector reports the Control Selector of the
			 * control that have issued the interrupt. */
			uint8 selector;
			/* The fifth field attribute specifies the type of control change:
			 * 0x00 is Control value change, 0x01 is Control info change,
			 * 0x02 is Control failure change
			 * and values 0x03 - 0xFF are reserved. */
			uint8 attribute;
			/* After that can be several value fields that can include values
			 * 0x00, 0x01 and 0x02. */
			uint8 value[0];
		} videocontrol_interface_as_orignator;
		struct videostreaming_interface_as_originator {
			/* If the originator bitfield value is 2 (VideoStreaming interface)
			 * then third field of the status_packet_format is event field that
			 * can have value 0x00 (Button Press)
			 * or values 0x01 - 0xFF (Stream errors). */
			uint8 event;
			/* If event value is 0x00 (Button Press) there is only one value
			 * field. It can include values 0x00 (Button released)
			 * or 0x01 (Button pressed). If event value was some Stream error
			 * value, then there can be several value fields. */
			uint8* value;
		} videostreaming_interface_as_originator;
	} originators;
} _PACKED usb_video_status_packet_format;


typedef struct {
	uint8 length; // 9 bytes
	uint8 descriptor_type;
	uint8 interface_number;
	uint8 alternate_setting;
	uint8 num_endpoints;
	uint8 interface_class;
	uint8 interface_sub_class;
	uint8 interface_protocol;
		// Not used. Must be set to USB_VIDEO_PC_PROTOCOL_UNDEFINED
	uint8 interface;
} _PACKED usb_video_standard_vc_interface_descriptor;


typedef struct {
	uint8 length; // 8 bytes
	uint8 descriptor_type;
	uint8 first_interface;
	uint8 interface_count;
	uint8 function_class;
	uint8 function_sub_class;
	uint8 function_protocol;
		// Not used. Must be set to USB_VIDEO_PC_PROTOCOL_UNDEFINED
	uint8 function;
} _PACKED usb_video_standard_video_interface_collection_iad;


typedef struct {
	uint8 length; // 6 bytes + nr_in_pins
	uint8 descriptor_type;
	uint8 descriptor_sub_type;
	uint8 unit_id;
	uint8 nr_in_pins;
	uint8* source_id;
	uint8 selector;
} _PACKED usb_video_selector_unit_descriptor;


typedef struct {
	uint8 header_length;
	struct header_info {
		B_LBITFIELD8_8 (
				frame_id: 1,
				end_of_frame: 1,
				presentation_time: 1,
				source_clock_reference: 1,
				reserved: 1,
				still_image: 1,
				error: 1,
				end_of_header: 1
			     );
	} _header_info;
} _PACKED usb_video_payload_header_format;


typedef struct {
	uint32 presentation_time;
	uint8 source_clock[6];
} _PACKED usb_video_payload_header_format_extended_fields;


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


typedef struct {
	uint8 length; // 24 bytes + nr_in_pins + control_size
	uint8 descriptor_type;
	uint8 descriptor_sub_type;
	uint8 unit_id;
	uint8 guid_extension_code[16];
	uint8 num_controls;
	uint8 nr_in_pins;
	uint8 source_id[0];
#if 0
	// Remaining part of the structure can't be encoded because source_id has
	// a variable size
	uint8 control_size;
	struct controls {
		B_LBITFIELD8_8 (
				vendor_specific0 : 1,
				vendor_specific1 : 1,
				vendor_specific2 : 1,
				vendor_specific3 : 1,
				vendor_specific4 : 1,
				vendor_specific5 : 1,
				vendor_specific6 : 1,
				vendor_specific7 : 1
			     );
	} * _controls;
	uint8 extension;
#endif
} _PACKED usb_video_extension_unit_descriptor;


typedef struct {
	uint8 length; // 7 bytes
	uint8 descriptor_type;
	struct end_point_address {
		B_LBITFIELD8_3 (
				endpoint_number: 4, // Determined by the designer
				reserved: 3, // Reserved. Set to zero.
				direction: 1 // 0 = OUT, 1 = IN
			     );
	} _end_point_address;
	struct attributes {
		B_LBITFIELD8_3 (
				transfer_type: 2, // Must be set to 11 (Interrupt)
				synchronization_type: 2, // Must be set to 00 (None)
				reserved: 4 // Reserved. Must be set to zero
			     );
	} _attributes;
	uint16 max_packet_size;
	uint8 interval;
} _PACKED usb_video_vc_interrupt_endpoint_descriptor;


typedef struct {
	uint8 length; // 9 bytes
	uint8 descriptor_type;
	uint8 interface_number;
	uint8 alternate_setting;
	uint8 num_endpoints;
	uint8 interface_class;
	uint8 interface_sub_class;
	uint8 interface_protocol;
		// Not used. Must be set to USB_VIDEO_PC_PROTOCOL_UNDEFINED
	uint8 interface;
} _PACKED usb_video_standard_vs_interface_descriptor;


typedef struct {
	uint8 length; // 5 bytes
	uint8 descriptor_type;
	uint8 descriptor_sub_type;
	uint16 max_transfer_size;
} _PACKED usb_video_class_specific_vc_interrupt_endpoint_descriptor;


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


typedef struct {
	uint8 length; // 13 bytes + (num_formats*control_size)
	uint8 descriptor_type;
	uint8 descriptor_sub_type;
	uint8 num_formats;
	uint16 total_length;
	struct endpoint_address {
		B_LBITFIELD8_3 (
				endpoint_number: 4, // Determined by the designer
				reserved: 3, // Set to zero.
				direction: 1 // 0 = OUT, 1 = IN
			     );
	}  _endpoint_address;
	struct info {
		B_LBITFIELD8_2 (
				dynamic_format_change_support: 1,
				reserved: 7 // Set to zero.
			     );
	} _info;
	uint8 terminal_link;
	uint8 still_capture_method;
	uint8 trigger_support;
	uint8 trigger_usage;
	uint8 control_size;
	struct ma_controls {
		// For four first bits, a bit set to 1 indicates that the named field
		// is supported by the Video Probe and Commit Control when
		// its format_index is 1:
		B_LBITFIELD8_7 (
				key_frame_rate: 1,
				p_frame_rate: 1,
				comp_quality: 1,
				comp_window_size: 1,
				// For the next two bits, a bit set to 1 indicates that the named
				// control is supported by the device when
				// format_index is 1:
				generate_key_frame: 1,
				update_frame_segment: 1,
				reserved: 2 // (control_size*8-1): Set to zero.
			     );
	} * _ma_controls;
} _PACKED usb_video_class_specific_vs_interface_input_header_descriptor;



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


typedef struct {
	uint8 length; // 9 + (num_formats*control_size)
	uint8 descriptor_type;
	uint8 descriptor_sub_type;
	uint8 num_formats;
	uint16 total_length;
	struct endpoint_address {
		B_LBITFIELD8_3 (
				endpoint_number: 4, // Determined by the designer
				reserved: 3, // Set to zero.
				direction: 1 // 0 = OUT
			     );
	} _endpoint_address;
	uint8 terminal_link;
	uint8 control_size;
	struct ma_controls {
		// For four first bits, a bit set to 1 indicates that the named field
		// is supported by the Video Probe and Commit Control when its
		// format_index is 1:
		B_LBITFIELD8_5 (
				key_frame_rate: 1,
				p_frame_rate: 1,
				comp_quality: 1,
				comp_window_size: 1,
				reserved: 4 // (control_size*8-1) Set to zero.
			     );
	} * _ma_controls;
} _PACKED usb_video_class_specific_vs_interface_output_header_descriptor;


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
	union {
#if B_HOST_IS_LENDIAN
		struct {
			struct control_a {
				B_LBITFIELD16_16 (
						brightness: 1,
						contrast: 1,
						hue: 1,
						saturation: 1,
						sharpness: 1,
						gamma: 1,
						white_balance_temperature: 1,
						white_balance_component: 1,
						backlight_compensation: 1,
						gain: 1,
						power_line_frequency: 1,
						hue_auto: 1,
						white_balance_temperature_auto: 1,
						white_balance_component_auto: 1,
						digital_multiplier: 1,
						digital_multiplier_limit: 1
						);
			} _control_a;
			struct control_b {
				B_LBITFIELD16_3 (
						analog_video_standard: 1,
						analog_video_lock_status: 1,
						reserved: 14 // Reserved. Se to zero.
						);
			} _control_b;
		} _controls;
#else
		struct {
			struct control_b {
				B_LBITFIELD16_3 (
					analog_video_standard: 1,
					analog_video_lock_status: 1,
					reserved: 14 // Reserved. Se to zero.
				);
			} _control_b;
			struct control_a {
				B_LBITFIELD16_16 (
					brightness: 1,
					contrast: 1,
					hue: 1,
					saturation: 1,
					sharpness: 1,
					gamma: 1,
					white_balance_temperature: 1,
					white_balance_component: 1,
					backlight_compensation: 1,
					gain: 1,
					power_line_frequency: 1,
					hue_auto: 1,
					white_balance_temperature_auto: 1,
					white_balance_component_auto: 1,
					digital_multiplier: 1,
					digital_multiplier_limit: 1
				);
			} _control_a;
		} _controls;
#endif
		uint8_t controls[4];
	};
	uint8 processing;
	union {
		struct {
			B_LBITFIELD8_8 (
				none: 1,
				ntsc_525_60: 1,
				pal_625_50: 1,
				secam_625_50: 1,
				ntsc_625_50: 1,
				pal_525_60: 1,
				reserved6: 1, // Reserved. Set to zero.
				reserved7: 1  // Reserved. Set to zero.
			);
		} _video_standards;
		uint8_t video_standards;
	};
} _PACKED usb_video_processing_unit_descriptor;


typedef struct {
	uint8 length; // 15 + control_size bytes
	uint8 descriptor_type;
	uint8 descriptor_sub_type;
	uint8 terminal_id;
	uint16 terminal_type;
	uint8 assoc_terminal;
	uint8 terminal;
	uint16 objective_focal_length_min;
	uint16 objective_focal_length_max;
	uint16 ocular_focal_length;
	uint8 control_size;
#if B_HOST_IS_LENDIAN
	struct controls {
		struct control_a {
			B_LBITFIELD16_16 (
					scanning_mode: 1,
					auto_exposure_mode: 1,
					auto_exposure_priority: 1,
					exposure_time_absolute: 1,
					exposure_time_relative: 1,
					focus_absolute: 1,
					focus_relative: 1,
					iris_absolute: 1,
					iris_relative: 1,
					zoom_absolute: 1,
					zoom_relative: 1,
					pan_tilt_absolute: 1,
					pan_tilt_relative: 1,
					roll_absolute: 1,
					roll_relative: 1,
					reserved15: 1
				    );
		} _control_a;
		struct control_b {
			B_LBITFIELD16_4 (
					reserved16: 1,
					focus_auto: 1,
					privacy: 1,
					// D19...(control_size*8-1): Reserved, set to zero.
					reserved: 13
				   );
		} _contorl_b;
	} _controls;
#else
	struct controls {
		struct control_b {
			B_LBITFIELD16_4 (
					reserved16: 1,
					focus_auto: 1,
					privacy: 1,
					// D19...(control_size*8-1): Reserved, set to zero.
					reserved: 13
				   );
		} _contorl_b;
		struct control_a {
			B_LBITFIELD16_16 (
					scanning_mode: 1,
					auto_exposure_mode: 1,
					auto_exposure_priority: 1,
					exposure_time_absolute: 1,
					exposure_time_relative: 1,
					focus_absolute: 1,
					focus_relative: 1,
					iris_absolute: 1,
					iris_relative: 1,
					zoom_absolute: 1,
					zoom_relative: 1,
					pan_tilt_absolute: 1,
					pan_tilt_relative: 1,
					roll_absolute: 1,
					roll_relative: 1,
					reserved15: 1
				    );
		} _control_a;
	} _controls;
#endif
} _PACKED usb_video_camera_terminal_descriptor;


typedef struct {
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
} _PACKED usb_video_frame_descriptor;


typedef struct {
	uint8 length; // 34 bytes
	struct hint {
		B_LBITFIELD16_5 (
				frame_interval: 1,
				key_frame_rate: 1,
				p_frame_rate: 1,
				comp_quality: 1,
				reserved: 12
			   );
	} _hint;
	uint8 format_index;
	uint8 frame_index;
	uint32 frame_interval;
	uint16 key_frame_rate;
	uint16 p_frame_rate;
	uint16 comp_quality;
	uint16 comp_window_size;
	uint16 delay;
	uint32 max_video_frame_size;
	uint32 max_payload_transfer_size;
	uint32 clock_frequency;
	struct framing_info {
		B_LBITFIELD8_3 (
				is_frame_id_required: 1,
				is_end_of_frame_present: 1,
				reserved: 6
			     );
	} _framing_info;
	uint8 prefered_version;
	uint8 min_version;
	uint8 max_version;
} _PACKED usb_video_video_probe_and_commit_controls;


typedef struct {
	uint8 length; // 11 bytes
	uint8 frame_index;
	uint8 compression_index;
	uint32 max_video_frame_size;
	uint32 max_payload_transfer_size;
} _PACKED usb_video_video_still_probe_control_and_still_commit_control;


typedef struct {
	// 10 bytes + (4 * num_image_size_patterns) - 4 + num_compression_pattern
	uint8 length;
	uint8 descriptor_type;
	uint8 descriptor_sub_type;
	uint8 endpoint_address;
	uint8 num_image_size_patterns;
	struct pattern_size {
		uint16 width;
		uint16 height;
	} * _pattern_size;
	uint8 num_compression_pattern;
	uint8* compression;
} _PACKED usb_video_still_image_frame_descriptor;


typedef struct {
	uint8 length; // 7 bytes
	struct endpoint_address {
		B_LBITFIELD8_3 (
				endpoint_number: 4, // Determined by the designer
				reserved: 3,
				direction: 1 // Set to 1 = IN endpoint)
			);
	} _endpoint_address;
	struct attributes {
		B_LBITFIELD8_2 (
				transfer_type: 2, // Set to 10 = Bulk
				reserved: 6
			     );
	} _attributes;
	uint16 max_packet_size;
	uint8 interval;
} _PACKED usb_video_standard_vs_bulk_still_image_data_endpoint_descriptor;


typedef struct {
	uint8 length; // 6 bytes
	uint8 descriptor_type;
	uint8 descriptor_sub_type;
	uint8 color_primaries;
	uint8 transfer_characteristics;
	uint8 matrix_coefficients;
} _PACKED usb_video_color_matching_descriptor;


typedef struct {
	uint8 length; // 7 bytes
	uint8 descriptor_type;
	struct endpoint_address {
		B_LBITFIELD8_3 (
				endpoint_number: 4, // Determined by the designer
				reserved: 3, // Reset to zero.
				direction: 1 // 0 = OUT endpoint, 1 = IN endpoint
			     );
	} _endpoint_address;
	struct attributes {
		B_LBITFIELD8_3 (
				transfer_type: 2, // 01 = isochronous
				synchronization_type: 2, // 01 = asynchronous
				reserved: 4
			     );
	} _attributes;
	uint16 max_packet_size;
	uint8 interval;
} _PACKED usb_video_std_vs_isochronous_video_data_endpoint_descriptor;


typedef struct {
	uint8 length; // 7 bytes
	uint8 descriptor_type;
	struct endpoint_address {
		B_LBITFIELD8_3 (
				endpoint_number: 4, // Determined by the designer
				reserved: 3, // Reset to zero.
				direction: 1 // 0 = OUT endpoint
			     );
	} _endpoint_address;
	struct attributes {
		B_LBITFIELD8_2 (
				transfer_type: 2, // Set to 10 = Bulk
				reserved: 6
			     );
	} _attributes;
	uint16 max_packet_size;
	uint8 interval;
} _PACKED usb_video_standard_vs_bulk_video_data_endpoint_descriptor;


#endif /* !USB_VIDEO_H */
