/* 	Copyright 2000 Be Incorporated. All Rights Reserved.
**	This file may be used under the terms of the Be Sample Code 
**	License.
*/

#ifndef _USB_AUDIO_H_
#define _USB_AUDIO_H_

/* See: Universal Serial Bus Device Class Definition for Audio Devices, Release 1.0 */


/* A.1 Audio Interface Class Code */
#define AC_AUDIO 0x01

/* A.2 Audio Interface Subclass Codes */
#define AC_SUBCLASS_UNDEFINED 0x00
#define AC_AUDIOCONTROL 0x01
#define AC_AUDIOSTREAMING 0x02
#define AC_MIDISTREAMING 0x03

/* A.3 Audio Interface Protocol Codes */
#define AC_PR_PROTOCOL_UNDEFINED 0x00

/* A.4 Audio Class-Specific Descriptor Types */
#define AC_CS_UNDEFINED 0x20
#define AC_CS_DEVICE 0x21
#define AC_CS_CONFIGURATION 0x22
#define AC_CS_STRING 0x23
#define AC_CS_INTERFACE 0x24
#define AC_CS_ENDPOINT 0x25

/* A.5 Audio Class-Specific AC Interface Descriptor Subtypes */
#define AC_AC_DESCRIPTOR_UNDEFINED 0x00
#define AC_HEADER 0x01
#define AC_INPUT_TERMINAL 0x02
#define AC_OUTPUT_TERMINAL 0x03
#define AC_MIXER_UNIT 0x04
#define AC_SELECTOR_UNIT 0x05
#define AC_FEATURE_UNIT 0x06
#define AC_PROCESSING_UNIT 0x07
#define AC_EXTENSION_UNIT 0x08

/* A.6 Audio Class-Specific AS Interface Descriptor Subtypes */
#define AC_AS_DESCRIPTOR_UNDEFINED 0x00
#define AC_AS_GENERAL 0x01
#define AC_FORMAT_TYPE 0x02
#define AC_FORMAT_SPECIFIC 0x03

/* A.7 Processing Unit Process Types */
#define AC_PROCESS_UNDEFINED 0x00
#define AC_UPDOWNMIX_PROCESS 0x01
#define AC_DOLBY_PROLOGIC_PROCESS 0x02
#define AC_3D_STEREO_EXTENDER_PROCESS 0x03
#define AC_REVERBERATION_PROCESS 0x04
#define AC_CHORUS_PROCESS 0x05
#define AC_DYN_RANGE_COMP_PROCESS 0x07

/* A.8 Audio Class-Specific Endpoint Descriptor Subtypes */
#define AC_DESCRIPTOR_UNDEFINED 0x00
#define AC_EP_GENERAL 0x01

/* A.9 Audio Class-Specific Request Codes */
#define AC_REQUEST_CODE_UNDEFINED 0x00
#define AC_SET_CUR 0x01
#define AC_GET_CUR 0x81
#define AC_SET_MIN 0x02
#define AC_GET_MIN 0x82
#define AC_SET_MAX 0x03
#define AC_GET_MAX 0x83
#define AC_SET_RES 0x04
#define AC_GET_RES 0x84
#define AC_SET_MEM 0x05
#define AC_GET_MEM 0x85
#define AC_GET_STAT 0xFF

/* A.10.1 Terminal Control Selectors */
#define AC_TE_CONTROL_UNDEFINED 0x00
#define AC_COPY_PROTECT_CONTROL 0x01

/* A.10.2 Feature Unit Control Selectors */
#define AC_FU_CONTROL_UNDEFINED 0x00
#define AC_MUTE_CONTROL 0x01
#define AC_VOLUME_CONTROL 0x02
#define AC_BASS_CONTROL 0x03
#define AC_MID_CONTROL 0x04
#define AC_TREBLE_CONTROL 0x05
#define AC_GRAPHIC_EQUALIZER_CONTROL 0x06
#define AC_AUTOMATIC_GAIN_CONTROL 0x07
#define AC_DELAY_CONTROL 0x08
#define AC_BASS_BOOST_CONTROL 0x09
#define AC_LOUDNESS_CONTROL 0x0A

/* A.10.3.1 Up/Down-mix Processing Unit Control Selectors */
#define AC_UD_CONTROL_UNDEFINED 0x00
#define AC_UD_ENABLED_CONTROL 0x01
#define AC_UD_MODE_SELECT_CONTROL 0x02

/* A.10.3.2 Dolby Prologic(tm) Processing Unit Control Selectors */
#define AC_DP_CONTROL_UNDEFINED 0x00
#define AC_DP_ENABLE_CONTROL 0x01
#define AC_DP_MODE_SELECT_CONTROL 0x02

/* A.10.3.3 3D Stereo Extender Processing Unit Control Selectors */
#define AC_3D_CONTROL_UNDEFINED 0x00
#define AC_3D_ENABLED_CONTROL 0x01
#define AC_3D_SPACIOUSNESS_CONTROL 0x03

/* A.10.3.4 Reverberation Processing Unit Control Selectors */
#define AC_RV_CONTROL_UNDEFINED 0x00
#define AC_RV_ENABLE_CONTROL 0x01
#define AC_RV_REVERB_LEVEL_CONTROL 0x02
#define AC_RV_REVERB_TIME_CONTROL 0x03
#define AC_RV_REVERB_FEEDBACK_CONTROL 0x04

/* A.10.3.5 Chorus Processing Unit Control Selectors */
#define AC_CH_CONTROL_UNDEFINED 0x00
#define AC_CH_ENABLE_CONTROL 0x01
#define AC_CHORUS_LEVEL_CONTROL 0x02
#define AC_CHORUS_RATE_CONTROL 0x03
#define AC_CHORUS_DETH_CONTROL 0x04

/* A.10.3.6 Dynamic Range Compressor Processing Unit Control Selectors */
#define AC_DR_CONTROL_UNDEFINED 0x00
#define AC_DR_ENABLE_CONTROL 0x01
#define AC_COMPRESSION_RATE_CONTROL 0x02
#define AC_MAXAMPL_CONTROL 0x03
#define AC_THRESHOLD_CONTROL 0x04
#define AC_ATTACK_TIME 0x05
#define AC_RELEASE_TIME 0x06

/* A.10.4 Extension Unit Control Selectors */
#define AC_XU_CONTROL_UNDEFINED 0x00
#define AC_XU_ENABLE_CONTROL 0x01

/* A.10.5 Endpoint Control Selectors */
#define AC_EP_CONTROL_UNDEFINED 0x00
#define AC_SAMPLING_FREQ_CONTROL 0x01
#define AC_PITCH_CONTROL 0x02

typedef struct 
{
	uint8 length;
	uint8 type;
	uint8 subtype;
	uint8 unit_id;
	uint8 source_id;
	uint8 control_size;
	uint16 controls[0];
} _PACKED usb_feature_unit_descr;

typedef struct 
{
	uint8 length;
	uint8 type;
	uint8 subtype;
	uint8 format_type;
	uint8 num_channels;
	uint8 subframe_size;
	uint8 bit_resolution;
	uint8 sample_freq_type;
	uint8 lower_sample_freq[3];
	uint8 upper_sample_freq[3];
} _PACKED usb_format_type_descr;

#endif
