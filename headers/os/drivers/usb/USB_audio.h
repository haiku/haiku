#ifndef USB_AUDIO_H
#define USB_AUDIO_H

// (Partial) USB Device Class Definition for Audio Devices
// R1: Release 1.0 March 18, 1998
// R2: Release 2.0 May 31, 2006

#include <SupportDefs.h>

#define USB_AUDIO_DEVICE_CLASS 			0x01
#define USB_AUDIO_CLASS_VERSION			0x0100

enum {
	USB_AUDIO_INTERFACE_AUDIO_CLASS		= 0x01
};

enum { // Audio Interface Subclasses	
	USB_AUDIO_INTERFACE_UNDEFINED_SUBCLASS =	0x00, 
	USB_AUDIO_INTERFACE_AUDIOCONTROL_SUBCLASS =	0x01,
	USB_AUDIO_INTERFACE_AUDIOSTREAMING_SUBCLASS,		//
	USB_AUDIO_INTERFACE_MIDISTREAMING_SUBCLASS			// 
};

enum { // Audio Interface Protocol Codes	
	USB_AUDIO_PROTOCOL_UNDEFINED =	0x00 
};

enum { // Audio Interface Protocol Codes	
	USB_AUDIO_CS_UNDEFINED				= 0x20,
	USB_AUDIO_CS_DEVICE					= 0x21,
	USB_AUDIO_CS_CONFIGURATION			= 0x22,
	USB_AUDIO_CS_STRING					= 0x23,
	USB_AUDIO_CS_INTERFACE				= 0x24,
	USB_AUDIO_CS_ENDPOINT				= 0x25
};


enum { // Audio Class-Specific AudioControl Interface descriptor subtypes
	USB_AUDIO_AC_DESCRIPTOR_UNDEFINED	= 0x00,
	USB_AUDIO_AC_HEADER					= 0x01,
	USB_AUDIO_AC_INPUT_TERMINAL			= 0x02,
	USB_AUDIO_AC_OUTPUT_TERMINAL		= 0x03,
	USB_AUDIO_AC_MIXER_UNIT				= 0x04,
	USB_AUDIO_AC_SELECTOR_UNIT			= 0x05,
	USB_AUDIO_AC_FEATURE_UNIT			= 0x06,
	USB_AUDIO_AC_PROCESSING_UNIT		= 0x07,
	USB_AUDIO_AC_EXTENSION_UNIT			= 0x08,
	USB_AUDIO_AC_EFFECT_UNIT_R2			= 0x07,
	USB_AUDIO_AC_PROCESSING_UNIT_R2		= 0x08,
	USB_AUDIO_AC_EXTENSION_UNIT_R2		= 0x09,
	USB_AUDIO_AC_CLOCK_SOURCE_R2		= 0x0A,
	USB_AUDIO_AC_CLOCK_SELECTOR_R2		= 0x0B,
	USB_AUDIO_AC_CLOCK_MULTIPLIER_R2	= 0x0C,
	USB_AUDIO_AC_SAMPLE_RATE_CONVERTER_R2 = 0x0D
};


// Class Specific Audio Control Interface Header
// R1: Table 4-2 p.37 / R2: Table 4-5 p.48 
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// USB_AUDIO_CS_INTERFACE
	uint8	descriptor_subtype; 	// USB_AUDIO_AC_HEADER
	uint16	bcd_release_no;
	union {
		struct {
			uint16	total_length;
			uint8	in_collection;
			uint8	interface_numbers[1];
		} _PACKED r1;

		struct {
			uint8	function_category;
			uint16	total_length;
			uint8	bm_controls;
		} _PACKED r2;
	};
} _PACKED usb_audiocontrol_header_descriptor;


// Input Terminal Descriptor
// R1: Table 4-3 p.39 / R2: Table 4-9, page 53
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// USB_AUDIO_CS_INTERFACE
	uint8	descriptor_subtype; 	// USB_AUDIO_AC_INPUT_TERMINAL
	uint8	terminal_id;
	uint16	terminal_type;
	uint8	assoc_terminal;
	union {
		struct {
			uint8	num_channels;
			uint8	channel_config;
			uint8	channel_names;
			uint8	terminal;
		} _PACKED r1;

		struct {
			uint8	clock_source_id;
			uint8	num_channels;
			uint32	channel_config;
			uint8	channel_names;
			uint16	bm_controls;
			uint8	terminal;
		} _PACKED r2;
	};
} _PACKED usb_audio_input_terminal_descriptor;


// Output Terminal Descriptor
// R1: Table 4-4 p.40 / R2: Table 4-10, page 54
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// USB_AUDIO_CS_INTERFACE
	uint8	descriptor_subtype; 	// USB_AUDIO_AC_OUTPUT_TERMINAL
	uint8	terminal_id;
	uint16	terminal_type;
	uint8	assoc_terminal;
	uint8	source_id;				
	union {
		struct {
			uint8	terminal;		
		} _PACKED r1;

		struct {
			uint8	clock_source_id;
			uint16	bm_controls;	
			uint8	terminal;		
		} _PACKED r2;
	};
} _PACKED usb_audio_output_terminal_descriptor;


// pseudo-descriptor for a section corresponding to logical output channels
// used in mixer, processing and extension descriptions.
typedef struct {
	uint8	num_output_pins; 		// number of mixer output pins
	uint16	channel_config;		 	// location of logical channels
	uint8	channel_names;		 	// id of name string of first logical channel
} _PACKED usb_audio_output_channels_descriptor_r1;

typedef struct {
	uint8	num_output_pins; 		// number of mixer output pins
	uint32	channel_config;		 	// location of logical channels
	uint8	channel_names;		 	// id of name string of first logical channel
} _PACKED usb_audio_output_channels_descriptor;


// Mixer Unit Descriptor
// R1: Table 4-5 p.41 / R2: Table 4-11, page 57
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// USB_AUDIO_CS_INTERFACE
	uint8	descriptor_subtype; 	// USB_AUDIO_AC_MIXER_UNIT
	uint8	unit_id;
	uint8	num_input_pins;
	uint8	input_pins[1];
	// use usb_audio_output_channels_descriptor to parse the rest
} _PACKED usb_audio_mixer_unit_descriptor;


// Selector Unit Descriptor
// R1: Table 4-6 p.43 / R2: Table 4-12, page 58
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// USB_AUDIO_CS_INTERFACE
	uint8	descriptor_subtype; 	// USB_AUDIO_AC_SELECTOR_UNIT
	uint8	unit_id;
	uint8	num_input_pins;
	uint8	input_pins[1];
	// uint8 selector_string;								
} _PACKED usb_audio_selector_unit_descriptor;


// Feature Unit Descriptor
// R1: Table 4-7 p.43 / R2: Table 4-13, page 59
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// USB_AUDIO_CS_INTERFACE
	uint8	descriptor_subtype; 	// USB_AUDIO_AC_FEATURE_UNIT
	uint8	unit_id;
	uint8	source_id;
	union {
		struct {
			uint8	control_size;	
			uint8	bma_controls[1];
		//	uint8	feature_string;
		} _PACKED r1;

		struct {
			uint32	bma_controls[1];
		//	uint8	feature_string;	
		} _PACKED r2;
	};
} _PACKED usb_audio_feature_unit_descriptor;


// Processing Unit Descriptor
// R1: Table 4-8 p.45 / R2: Table 4-20, page 66
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// USB_AUDIO_CS_INTERFACE
	uint8	descriptor_subtype; 	// USB_AUDIO_AC_PROCESSING_UNIT
	uint8	unit_id;		
	uint16	process_type;	
	uint8	num_input_pins;	
	uint8	input_pins[1];	
	// use usb_audio_output_channels_descriptor to parse the rest
// TODO - the bmControl!!!!
} _PACKED usb_audio_processing_unit_descriptor;

// Extension Unit Descriptor
// R1: Table 4-15, p. 56 / R2: Table 4-24, p.73
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// USB_AUDIO_CS_INTERFACE
	uint8	descriptor_subtype; 	// USB_AUDIO_AC_EXTENSION_UNIT
	uint8	unit_id;
	uint16	extension_code;
	uint8	num_input_pins;
	uint8	input_pins[1];
	// use usb_audio_output_channels_descriptor to parse the rest
} _PACKED usb_audio_extension_unit_descriptor;


enum { // Audio Class-Specific AudioStreaming Interface descriptor subtypes
	USB_AUDIO_AS_DESCRIPTOR_UNDEFINED	= 0x00,
	USB_AUDIO_AS_GENERAL				= 0x01,
	USB_AUDIO_AS_FORMAT_TYPE			= 0x02,
	USB_AUDIO_AS_FORMAT_SPECIFIC		= 0x03
};

// Class Specific Audio Streaming Interface Descriptor
// R1: Table 4-19 p.60 / R2: Table 4-27 p.76 
typedef struct {
	uint8	length;
	uint8	descriptor_type;		// USB_AUDIO_CS_INTERFACE
	uint8	descriptor_subtype; 	// USB_AUDIO_AS_GENERAL
	uint8	terminal_link;
	union {
		struct {
			uint8	delay;
			uint16	format_tag;
		} _PACKED r1;

		struct {
			uint8	bm_controls;
			uint8	format_type;
			uint32	bm_formats;	
			uint8	num_output_pins;
			uint32	channel_config;
			uint8	channel_names;
		} _PACKED r2;
	};
} _PACKED usb_audio_streaming_interface_descriptor;


// Class-specific As Isochronous Audio Data Endpoint descriptor
// R1: Table 4-21, p. 62 / R2: Table 4-34, page 87
typedef struct {
	uint8	length;	
	uint8	descriptor_type;		// USB_AUDIO_CS_ENDPOINT
	uint8	descriptor_subtype; 	// USB_AUDIO_EP_GENERAL
	uint8	attributes;	
	uint8	lock_delay_units;
	uint16	lock_delay;	
} _PACKED usb_audio_streaming_endpoint_descriptor;


// Sampling Rate are represented as 3-byte integer
typedef struct {
	uint8	bytes[3];
} _PACKED usb_audio_sampling_freq;


// Format Type I/II/III Descriptors
// T1: Table 2-1, p.10 etc.
// T2: Table 2-2, p.15 etc.
typedef struct {
	uint8 length;
	uint8 descriptor_type;				// USB_AUDIO_CS_INTERFACE
	uint8 descriptor_subtype;			// USB_AUDIO_AS_FORMAT_TYPE
	uint8 format_type;					// USB_AUDIO_FORMAT_TYPE_I/II/III
	union {
		struct {
			uint8 nr_channels;
			uint8 subframe_size;
			uint8 bit_resolution;
			uint8 sam_freq_type;
			usb_audio_sampling_freq sam_freqs[1];
		} _PACKED typeI;

		struct {
			uint16 max_bit_rate;
			uint16 samples_per_frame;
			uint8 sam_freq_type;
			usb_audio_sampling_freq sam_freqs[1];
		} _PACKED typeII;

		struct {
			uint8 nr_channels;
			uint8 subframe_size;
			uint8 bit_resolution;
			uint8 sam_freq_type;
			usb_audio_sampling_freq sam_freqs[1];
		} _PACKED typeIII;
	};
} _PACKED usb_audio_format_descriptor;


enum { // Processing Unit Process Types  (for USB_AUDIO_AC_PROCESSING_UNIT)
	USB_AUDIO_UPDOWNMIX_PROCESS	= 0x01,
	USB_AUDIO_DOLBY_PROLOGIC_PROCESS,
	USB_AUDIO_3D_STEREO_EXTENDER_PROCESS,
	USB_AUDIO_REVERBERATION_PROCESS,
	USB_AUDIO_CHORUS_PROCESS,
	USB_AUDIO_DYN_RANGE_COMP_PROCESS
};

enum { // Audio Class-Specific Request Codes
	USB_AUDIO_SET_CUR		= 0x01,
	USB_AUDIO_GET_CUR		= 0x81,
	USB_AUDIO_SET_MIN		= 0x02,
	USB_AUDIO_GET_MIN		= 0x82,
	USB_AUDIO_SET_MAX		= 0x03,
	USB_AUDIO_GET_MAX		= 0x83,
	USB_AUDIO_SET_RES		= 0x04,
	USB_AUDIO_GET_RES		= 0x84,
	USB_AUDIO_SET_MEM		= 0x05,
	USB_AUDIO_GET_MEM		= 0x85,
	USB_AUDIO_GET_STATUS	= 0xFF
};

enum { // Terminal Control Selectors
	USB_AUDIO_COPY_PROTECT_CONTROL = 0x01
};

// R2: A.17.5 Mixer Unit Control Selectors
enum {
	USB_AUDIO_MU_CONTROL_UNDEFINED		= 0x00,
	USB_AUDIO_MIXER_CONTROL				= 0x01,
	USB_AUDIO_CLUSTER_CONTROL			= 0x02,
	USB_AUDIO_UNDERFLOW_CONTROL			= 0x03,
	USB_AUDIO_OVERFLOW_CONTROL			= 0x04,
	USB_AUDIO_LATENCY_CONTROL			= 0x05
};

/* A.10.2 Feature Unit Control Selectors */
enum {
	USB_AUDIO_AC_FU_CONTROL_UNDEFINED	= 0x00,
	USB_AUDIO_MUTE_CONTROL				= 0x01,
	USB_AUDIO_VOLUME_CONTROL			= 0x02,
	USB_AUDIO_BASS_CONTROL				= 0x03,
	USB_AUDIO_MID_CONTROL				= 0x04,
	USB_AUDIO_TREBLE_CONTROL			= 0x05,
	USB_AUDIO_GRAPHIC_EQUALIZER_CONTROL	= 0x06,
	USB_AUDIO_AUTOMATIC_GAIN_CONTROL	= 0x07,
	USB_AUDIO_DELAY_CONTROL				= 0x08,
	USB_AUDIO_BASS_BOOST_CONTROL		= 0x09,
	USB_AUDIO_LOUDNESS_CONTROL			= 0x0A
};

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
enum {
	USB_AUDIO_EP_CONTROL_UNDEFINED			= 0x00,
	USB_AUDIO_SAMPLING_FREQ_CONTROL			= 0x01,
	USB_AUDIO_PITCH_CONTROL					= 0x02
};

/*
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
*/
// bitset for feature control bitmap
// Table 4-7, page 44, bmaControls field
enum {
	BMA_CTL_MUTE_R1			= 0X0001,
	BMA_CTL_VOLUME_R1		= 0X0002,
	BMA_CTL_BASS_R1			= 0X0004,
	BMA_CTL_MID_R1			= 0X0008,
	BMA_CTL_TREBLE_R1		= 0X0010,
	BMA_CTL_GRAPHEQ_R1		= 0X0020,
	BMA_CTL_AUTOGAIN_R1		= 0X0040,
	BMA_CTL_DELAY_R1		= 0X0080,
	BMA_CTL_BASSBOOST_R1	= 0X0100,
	BMA_CTL_LOUDNESS_R1		= 0X0200,
	// Release 2.0
	BMA_CTL_MUTE			= 0X00000003,
	BMA_CTL_VOLUME			= 0X0000000C,
	BMA_CTL_BASS			= 0X00000030,
	BMA_CTL_MID				= 0X000000C0,
	BMA_CTL_TREBLE			= 0X00000300,
	BMA_CTL_GRAPHEQ			= 0X00000C00,
	BMA_CTL_AUTOGAIN		= 0X00003000,
	BMA_CTL_DELAY			= 0X0000C000,
	BMA_CTL_BASSBOOST		= 0X00030000,
	BMA_CTL_LOUDNESS		= 0X000C0000,
	BMA_CTL_INPUTGAIN		= 0X00300000,
	BMA_CTL_INPUTGAINPAD	= 0X00C00000,
	BMA_CTL_PHASEINVERTER	= 0X03000000,
	BMA_CTL_UNDERFLOW		= 0X0C000000,
	BMA_CTL_OVERFLOW		= 0X30000000
};

// Table A-4, page 30
// Format Type Codes
enum {
	USB_AUDIO_FORMAT_TYPE_UNDEFINED		= 0x00,
	USB_AUDIO_FORMAT_TYPE_I				= 0x01,
	USB_AUDIO_FORMAT_TYPE_II			= 0x02,
	USB_AUDIO_FORMAT_TYPE_III			= 0x03
};

// Table A-1, page 29, wFormatTag
// Audio Data Format Type I codes
enum {
	USB_AUDIO_FORMAT_TYPE_I_UNDEFINED	= 0x0000,
	USB_AUDIO_FORMAT_PCM				= 0x0001,
	USB_AUDIO_FORMAT_PCM8				= 0x0002,
	USB_AUDIO_FORMAT_IEEE_FLOAT			= 0x0003,
	USB_AUDIO_FORMAT_ALAW				= 0x0004,
	USB_AUDIO_FORMAT_MULAW				= 0x0005
};


enum {
	// USB Terminal Types
	USB_AUDIO_UNDEFINED_USB_IO			= 0x100,
	USB_AUDIO_STREAMING_USB_IO			= 0x101,
	USB_AUDIO_VENDOR_USB_IO				= 0x1ff,
	// Input Terminal Types
	USB_AUDIO_UNDEFINED_IN				= 0x200,
	USB_AUDIO_MICROPHONE_IN				= 0x201,
	USB_AUDIO_DESKTOPMIC_IN				= 0x202,
	USB_AUDIO_PERSONALMIC_IN			= 0x203,
	USB_AUDIO_OMNI_MIC_IN				= 0x204,
	USB_AUDIO_MICS_ARRAY_IN				= 0x205,
	USB_AUDIO_PROC_MICS_ARRAY_IN		= 0x206,
	// Output Terminal Types
	USB_AUDIO_UNDEFINED_OUT				= 0x300,
	USB_AUDIO_SPEAKER_OUT				= 0x301,
	USB_AUDIO_HEAD_PHONES_OUT			= 0x302,
	USB_AUDIO_HMD_AUDIO_OUT				= 0x303,
	USB_AUDIO_DESKTOP_SPEAKER			= 0x304,
	USB_AUDIO_ROOM_SPEAKER				= 0x305,
	USB_AUDIO_COMM_SPEAKER				= 0x306,
	USB_AUDIO_LFE_SPEAKER				= 0x307,
	// Bi-directional Terminal Types
	USB_AUDIO_UNDEFINED_IO				= 0x400,
	USB_AUDIO_HANDSET_IO				= 0x401,
	USB_AUDIO_HEADSET_IO				= 0x402,
	USB_AUDIO_SPEAKER_PHONE_IO			= 0x403,
	USB_AUDIO_SPEAKER_PHONEES_IO		= 0x404,
	USB_AUDIO_SPEAKER_PHONEEC_IO		= 0x405,
	// Telephony Terminal Types
	USB_AUDIO_UNDEF_TELEPHONY_IO		= 0x500,
	USB_AUDIO_PHONE_LINE_IO				= 0x501,
	USB_AUDIO_TELEPHONE_IO				= 0x502,
	USB_AUDIO_DOWNLINE_PHONE_IO			= 0x503,
	// External Terminal Types
	USB_AUDIO_UNDEFINEDEXT_IO			= 0x600,
	USB_AUDIO_ANALOG_CONNECTOR_IO		= 0x601,
	USB_AUDIO_DAINTERFACE_IO			= 0x602,
	USB_AUDIO_LINE_CONNECTOR_IO			= 0x603,
	USB_AUDIO_LEGACY_CONNECTOR_IO		= 0x604,
	USB_AUDIO_SPDIF_INTERFACE_IO		= 0x605,
	USB_AUDIO_DA1394_STREAM_IO			= 0x606,
	USB_AUDIO_DV1394_STREAMSOUND_IO		= 0x607,
	USB_AUDIO_ADAT_LIGHTPIPE_IO			= 0x608,
	USB_AUDIO_TDIF_IO					= 0x609,
	USB_AUDIO_MADI_IO					= 0x60a,
	// Embedded Terminal Types
	USB_AUDIO_UNDEF_EMBEDDED_IO			= 0x700,
	USB_AUDIO_LC_NOISE_SOURCE_OUT		= 0x701,
	USB_AUDIO_EQUALIZATION_NOISE_OUT	= 0x702,
	USB_AUDIO_CDPLAYER_IN				= 0x703,
	USB_AUDIO_DAT_IO					= 0x704,
	USB_AUDIO_DCC_IO					= 0x705,
	USB_AUDIO_MINI_DISK_IO				= 0x706,
	USB_AUDIO_ANALOG_TAPE_IO			= 0x707,
	USB_AUDIO_PHONOGRAPH_IN				= 0x708,
	USB_AUDIO_VCR_AUDIO_IN				= 0x709,
	USB_AUDIO_VIDEO_DISC_AUDIO_IN		= 0x70a,
	USB_AUDIO_DVD_AUDIO_IN				= 0x70b,
	USB_AUDIO_TV_TUNER_AUDIO_IN			= 0x70c,
	USB_AUDIO_SAT_RECEIVER_AUDIO_IN		= 0x70d,
	USB_AUDIO_CABLE_TUNER_AUDIO_IN		= 0x70e,
	USB_AUDIO_DSS_AUDIO_IN				= 0x70f,
	USB_AUDIO_RADIO_RECEIVER_IN			= 0x710,
	USB_AUDIO_RADIO_TRANSMITTER_IN		= 0x711,
	USB_AUDIO_MULTI_TRACK_RECORDER_IO	= 0x712,
	USB_AUDIO_SYNTHESIZER_IO			= 0x713,
	USB_AUDIO_PIANO_IO					= 0x714,
	USB_AUDIO_GUITAR_IO					= 0x715,
	USB_AUDIO_DRUMS_IO					= 0x716,
	USB_AUDIO_INSTRUMENT_IO				= 0x717
};


#endif	// USB_AUDIO_H
