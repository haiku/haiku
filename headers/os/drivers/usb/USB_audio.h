#ifndef USB_AUDIO_H
#define USB_AUDIO_H

// (Partial) USB Class Definitions for Audio Devices, version 1.0
// Reference: http://www.usb.org/developers/devclass_docs/audio10.pdf

#include <BeBuild.h>	// for _PACKED definition

#define USB_AUDIO_DEVICE_CLASS 			0x01
#define USB_AUDIO_CLASS_VERSION			0x0100

enum {
	USB_AUDIO_INTERFACE_AUDIO_CLASS		= 0x01
};

enum { // Audio Interface Subclasses	
	USB_AUDIO_INTERFACE_AUDIOCONTROL_SUBCLASS =	0x01,	// 
	USB_AUDIO_INTERFACE_AUDIOSTREAMING_SUBCLASS,		//
	USB_AUDIO_INTERFACE_MIDISTREAMING_SUBCLASS			// 
};

enum { // Audio Class-Specific AudioControl Interface descriptor subtypes
	USB_AUDIO_AC_DESCRIPTOR_UNDEFINED				= 0x00,
	USB_AUDIO_AC_HEADER						= 0x01,
	USB_AUDIO_AC_INPUT_TERMINAL,
	USB_AUDIO_AC_OUTPUT_TERMINAL,
	USB_AUDIO_AC_MIXER_UNIT,
	USB_AUDIO_AC_SELECTOR_UNIT,
	USB_AUDIO_AC_FEATURE_UNIT,
	USB_AUDIO_AC_PROCESSING_UNIT,
	USB_AUDIO_AC_EXTENSION_UNIT
};

enum { // Audio Class-Specific AudioStreaming Interface descriptor subtypes
	USB_AUDIO_AS_GENERAL						= 0x01,
	USB_AUDIO_AS_FORMAT_TYPE,
	USB_AUDIO_AS_FORMAT_SPECIFIC
};

enum { // Processing Unit Process Types  (for USB_AUDIO_AC_PROCESSING_UNIT)
	USB_AUDIO_UPDOWNMIX_PROCESS	= 0x01,
	USB_AUDIO_DOLBY_PROLOGIC_PROCESS,
	USB_AUDIO_3D_STEREO_EXTENDER_PROCESS,
	USB_AUDIO_REVERBERATION_PROCESS,
	USB_AUDIO_CHORUS_PROCESS,
	USB_AUDIO_DYN_RANGE_COMP_PROCESS
};

// AudioControl request codes
#define USB_AUDIO_SET_CURRENT 	0x01
#define USB_AUDIO_GET_CURRENT	0x81
#define USB_AUDIO_SET_MIN 		0x02
#define USB_AUDIO_GET_MIN 		0x82
#define USB_AUDIO_SET_MAX 		0x03
#define USB_AUDIO_GET_MAX 		0x83
#define USB_AUDIO_SET_RES 		0x04
#define USB_AUDIO_GET_RES 		0x84
#define USB_AUDIO_SET_MEM 		0x05
#define USB_AUDIO_GET_MEM 		0x85
#define USB_AUDIO_GET_STATUS 	0xFF

enum { // Terminal Control Selectors
	USB_AUDIO_COPY_PROTECT_CONTROL = 0x01
};

/* A.10.2 Feature Unit Control Selectors */
#define AC_FU_CONTROL_UNDEFINED 0x00
#define USB_AUDIO_MUTE_CONTROL 0x01
#define USB_AUDIO_VOLUME_CONTROL 0x02
#define USB_AUDIO_BASS_CONTROL 0x03
#define USB_AUDIO_MID_CONTROL 0x04
#define USB_AUDIO_TREBLE_CONTROL 0x05
#define USB_AUDIO_GRAPHIC_EQUALIZER_CONTROL 0x06
#define USB_AUDIO_AUTOMATIC_GAIN_CONTROL 0x07
#define USB_AUDIO_DELAY_CONTROL 0x08
#define USB_AUDIO_BASS_BOOST_CONTROL 0x09
#define USB_AUDIO_LOUDNESS_CONTROL 0x0A

/* A.10.3.1 Up/Down-mix Processing Unit Control Selectors */
#define USB_AUDIO_UD_CONTROL_UNDEFINED 0x00
#define USB_AUDIO_UD_ENABLED_CONTROL 0x01
#define USB_AUDIO_UD_MODE_SELECT_CONTROL 0x02

/* A.10.3.2 Dolby Prologic(tm) Processing Unit Control Selectors */
#define USB_AUDIO_DP_CONTROL_UNDEFINED 0x00
#define USB_AUDIO_DP_ENABLE_CONTROL 0x01
#define USB_AUDIO_DP_MODE_SELECT_CONTROL 0x02

/* A.10.3.3 3D Stereo Extender Processing Unit Control Selectors */
#define USB_AUDIO_3D_CONTROL_UNDEFINED 0x00
#define USB_AUDIO_3D_ENABLED_CONTROL 0x01
#define USB_AUDIO_3D_SPACIOUSNESS_CONTROL 0x03

/* A.10.3.4 Reverberation Processing Unit Control Selectors */
#define USB_AUDIO_RV_CONTROL_UNDEFINED 0x00
#define USB_AUDIO_RV_ENABLE_CONTROL 0x01
#define USB_AUDIO_RV_REVERB_LEVEL_CONTROL 0x02
#define USB_AUDIO_RV_REVERB_TIME_CONTROL 0x03
#define USB_AUDIO_RV_REVERB_FEEDBACK_CONTROL 0x04

/* A.10.3.5 Chorus Processing Unit Control Selectors */
#define USB_AUDIO_CH_CONTROL_UNDEFINED 0x00
#define USB_AUDIO_CH_ENABLE_CONTROL 0x01
#define USB_AUDIO_CHORUS_LEVEL_CONTROL 0x02
#define USB_AUDIO_CHORUS_RATE_CONTROL 0x03
#define USB_AUDIO_CHORUS_DETH_CONTROL 0x04

/* A.10.3.6 Dynamic Range Compressor Processing Unit Control Selectors */
#define USB_AUDIO_DR_CONTROL_UNDEFINED 0x00
#define USB_AUDIO_DR_ENABLE_CONTROL 0x01
#define USB_AUDIO_COMPRESSION_RATE_CONTROL 0x02
#define USB_AUDIO_MAXAMPL_CONTROL 0x03
#define USB_AUDIO_THRESHOLD_CONTROL 0x04
#define USB_AUDIO_ATTACK_TIME 0x05
#define USB_AUDIO_RELEASE_TIME 0x06

/* A.10.4 Extension Unit Control Selectors */
#define USB_AUDIO_XU_CONTROL_UNDEFINED 0x00
#define USB_AUDIO_XU_ENABLE_CONTROL 0x01

/* A.10.5 Endpoint Control Selectors */
#define USB_AUDIO_EP_CONTROL_UNDEFINED 0x00
#define USB_AUDIO_SAMPLING_FREQ_CONTROL 0x01
#define USB_AUDIO_PITCH_CONTROL 0x02

typedef struct
{
	uint8 length;
	uint8 type;
	uint8 subtype;
	uint8 unit_id;
	uint8 source_id;
	uint8 control_size;
	uint16 controls[0];
} _PACKED usb_audio_feature_unit_descriptor;

typedef struct {
	uint8 length;
	uint8 type;
	uint8 subtype;
	uint8 format_type;
	uint8 num_channels;
	uint8 subframe_size;
	uint8 bit_resolution;
	uint8 sample_freq_type;
	uint8 sample_freq[3];	// [0] + [1] << 8 + [2] << 16
	//  if sample_freq_type != 1,
	// (sample_freq_type - 1) x uint8 sample_freq[3] follows... 
} _PACKED usb_audio_format_type_descriptor;

#define USB_AUDIO_FORMAT_TYPE_UNDEFINED	0x00
#define USB_AUDIO_FORMAT_TYPE_I			0x01
#define USB_AUDIO_FORMAT_TYPE_II		0x02
#define USB_AUDIO_FORMAT_TYPE_III		0x03

#endif	// USB_AUDIO_H
