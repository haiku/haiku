/*
 *	USB audio spec stuctures
 *  
 *  Based on the USB Device Class Definition for Audio Devices Release 1.0
 *  (March 18, 1998)
 *  
 *  And the USB Device Class Definition for Audio Formats Release 1.0
 *  (March 18, 1998)
 */

#ifndef __USB_AUDIO_SPEC_H__
#define __USB_AUDIO_SPEC_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 *	Audio Device Class Codes
 */

// Audio Control descriptors

// Audio Control Interface descriptors
#define UAS_AUDIO        			0x01

#define UAS_SUBCLASS_UNDEFINED  	0x00
#define UAS_AUDIOCONTROL			0x01
#define UAS_AUDIOSTREAMING 			0x02
#define UAS_MIDISTREAMING			0x03

#define UAS_PR_PROTOCOL_UNDEFINED	0x00

// Table A-4, page 99
#define UAS_CS_UNDEFINED   			0x20
#define UAS_CS_DEVICE				0x21
#define UAS_CS_CONFIGURATION		0x22
#define UAS_CS_STRING				0x23
#define UAS_CS_INTERFACE			0x24
#define UAS_CS_ENDPOINT				0x25

// Table A-5, page 100
#define UAS_AC_DESCRIPTOR_UNDEFINED	0x00
#define UAS_HEADER					0x01
#define UAS_INPUT_TERMINAL			0x02
#define UAS_OUTPUT_TERMINAL			0x03
#define UAS_MIXER_UNIT				0x04
#define UAS_SELECTOR_UNIT			0x05
#define UAS_FEATURE_UNIT			0x06
#define UAS_PROCESSING_UNIT			0x07
#define UAS_EXTENSION_UNIT			0x08

// Table A-6, page 100
#define UAS_AS_DESCRIPTOR_UNDEFINED	0x00
#define UAS_AS_GENERAL				0x01
#define UAS_FORMAT_TYPE				0x02
#define UAS_FORMAT_SPECIFIC			0x03

// Audio Class-Specific Endpoint Descriptor Subtypes
// Table A-8, page 101
#define UAS_DESCRIPTOR_UNDEFINED	0x00
#define UAS_EP_GENERAL				0x01

// Audio Class-Specific Request Codes
#define UAS_REQUEST_CODE_UNDEFINED	0x00
#define UAS_SET_CUR					0x01
#define UAS_GET_CUR					0x81
#define UAS_SET_MIN					0x02
#define UAS_GET_MIN					0x82
#define UAS_SET_MAX					0x0x
#define UAS_GET_MAX					0x83
#define UAS_SET_RES					0x04
#define UAS_GET_RES					0x84
#define UAS_SET_MEM					0x05
#define UAS_GET_MEM					0x85
#define UAS_GET_STAT				0xff

#define UAS_bmReqType_00100001B		0x21
#define UAS_bmReqType_00100010B		0x22
#define UAS_bmReqType_10100001B		0xa1
#define UAS_bmReqType_10100010B		0xa2

// Table A-10.2, page 102
#define UAS_FU_CONTROL_UNDEFINED		0x00
#define UAS_MUTE_CONTROL				0x01
#define UAS_VOLUME_CONTROL				0x02
#define UAS_BASS_CONTROL				0x03
#define UAS_MID_CONTROL					0x04
#define UAS_TREBLE_CONTROL				0x05
#define UAS_GRAPHIC_EQUALIZER_CONTROL	0x06
#define UAS_AUTOMATIC_GAIN_CONTROL		0x07
#define UAS_DELAY_CONTROL				0x08
#define UAS_BASS_BOOST_CONTROL			0x09
#define UAS_LOUDNESS_CONTROL			0x0a

// Endpoint control selectors
// Table A-10.5, page 104
#define UAS_EP_CONTROL_UNDEFINED		0x00
#define UAS_SAMPLING_FREQ_CONTROL		0x01
#define UAS_PITCH_CONTROL				0x02

// header descriptor
typedef struct {
	uint8	length;				
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// HEADER
	uint16	bcd_release_no;			// Audio Device Class Specification relno
	uint16	total_length;			// 
	uint8	in_collection;			// # of audiostreaming units
	uint8	interface_numbers[1];	// or more
} _PACKED usb_audiocontrol_header_descriptor;

// input terminal descriptor
// Table 4-3, page 39 
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// INPUT_TERMINAL
	uint8	terminal_id;			// 
	uint16	terminal_type;			// 0x0101 ?
	uint8	assoc_terminal;			// OT terminal ID of corresponding outp
	uint8	num_channels;			// stereo = 2
	uint8	channel_config;			// spatial location of two channels (bitmap 0x03 is plain stereo)
	uint8	channel_names;			// index of string descr, name of first logical channel
	uint8	terminal;				// index of string descr, name of Input Terminal
} _PACKED usb_input_terminal_descriptor;

// output terminal descriptor
// Table 4-4, page 40 
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// OUTPUT_TERMINAL
	uint8	terminal_id;			// 
	uint16	terminal_type;			// 0x0101 ?
	uint8	assoc_terminal;			// OT terminal ID of corresponding outp
	uint8	source_id;				// ID of the unit or terminal to which this terminal is connected
	uint8	terminal;				// index of string descr, name of Input Terminal
} _PACKED usb_output_terminal_descriptor;

// selector unit descriptor
// Table 4-6, page 43
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// SELECTOR_UNIT
	uint8	unit_id;				// unique within audio function
	uint8	num_input_pins;			// 
	uint8	input_pins[2];			// 
	uint8	selector_string[1];		// YUK - doesn't work if num_input_pins!=2 - in that case, add (num_input_pins-2) to the index of this array...
} _PACKED usb_selector_unit_descriptor;
 
// feature unit descriptor
// Table 4-7, page 43
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// FEATURE_UNIT
	uint8	unit_id;				// unique within audio function
	uint8	source_id;				// id of the unit or terminal to which this unit is connected
	uint8	control_size;			// bma_controls is 2 bytes per element
	uint16	master_bma_control;		// only if control_size is 2
	uint16	bma_controls[2];		// if control_size = 2 and two channels
	uint8	feature_string;		    // if control_size = 2 and ch = 2
} _PACKED usb_feature_unit_descriptor;


// Audio Streaming (as) descriptors


// Class-specific AS interface descriptor
// Table 4-19, page 60
typedef struct {
	uint8	length;					// 7
	uint8	descriptor_type;		// CS_INTERFACE descriptor type (0x24)
	uint8	descriptor_subtype; 	// UAS_AS_GENERAL
	uint8	terminal_link;			// terminal ID to which this endp is connected
	uint8	delay;					// delay in # of frames
	uint16	format_tag;				// wFormatTag, 0x0001 = PCM
} _PACKED usb_as_interface_descriptor;

// Class-specific As Isochronous Audio Data Endpoint descriptor
// Table 4-21, page 62
typedef struct {
	uint8	length;					// 7
	uint8	descriptor_type;		// UAS_CS_ENDPOINT descriptor type (0x25)
	uint8	descriptor_subtype; 	// UAS_EP_GENERAL
	uint8	attributes;				// d0=samfq d1=pitch d7=maxpacketsonly
	uint8	lock_delay_units;		// 1=ms 2=decpcmsampl
	uint16	lock_delay;				// time for endp to lock internal clock recovery circuitry
} _PACKED usb_as_cs_endpoint_descriptor;



/* 3-byte integer */
typedef struct {
	uint8	data[3];
} _PACKED usb_triplet;

// and 

/*
 * Audio data formats spec
 */

// constants
// Table A-1, page 29, wFormatTag
// Audio Data Format Type I codes
#define UAF_TYPE_I_UNDEFINED		0x0000
#define UAF_PCM						0x0001
#define UAF_PCM8					0x0002
#define UAF_IEEE_FLOAT				0x0003
#define UAF_ALAW					0x0004
#define UAF_MULAW					0x0005

// Table A-4, page 30
// Format Type Codes
#define	UAF_FORMAT_TYPE_UNDEFINED	0x00
#define	UAF_FORMAT_TYPE_I			0x01
#define	UAF_FORMAT_TYPE_II			0x02
#define	UAF_FORMAT_TYPE_III			0x03 // Not _II again I guess..

// Table 2-2 and 2-3, page 10
typedef struct {
	uint8	lower_sam_freq[3];
	uint8	upper_sam_freq[3];
} _PACKED usb_audio_continuous_freq_descr;

typedef struct {
	uint8	sam_freq[1][3];
} _PACKED usb_audio_discrete_freq_descr;

typedef union {
	usb_audio_continuous_freq_descr cont;
	usb_audio_discrete_freq_descr   discr;
} _PACKED usb_audio_sam_freq_descr;

// Table 2-1, page 10
typedef struct {
	uint8 length;						// 0e for 						
	uint8 descriptor_type;				// UAS_CS_INTERFACE (0x24)
	uint8 descriptor_subtype;			// UAS_FORMAT_TYPE (0x02)
	uint8 format_type;					// UAF_FORMAT_TYPE_I (0x01)
	uint8 nr_channels;					// hopefully 2
	uint8 subframe_size;				// 1, 2, or 4 bytes
	uint8 bit_resolution;				// 8, 16 or 20 bits
	uint8 sam_freq_type;				// 0 == continuous, 1 == a fixed number of discrete sam freqs
	usb_audio_sam_freq_descr	sf;		// union
//	uint8 sam_freq[12 * 3];
} _PACKED usb_type_I_format_descriptor;

#ifdef __cplusplus
}
#endif

#endif
