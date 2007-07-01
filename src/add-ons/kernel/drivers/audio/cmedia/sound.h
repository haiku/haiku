/* ++++++++++
	$Source: /home/cltien/cvs/BeOS/inc/sound.h,v $
	$Revision: 1.1.1.1 $
	$Author: cltien $
	$Date: 1999/10/12 18:38:08 $

	Data structures and control calls for using the sound driver
+++++ */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _SOUND_H
#define _SOUND_H

#ifndef _DRIVERS_H
#include <Drivers.h>
#endif

enum adc_source {
  line=0, aux1, mic, loopback
};

enum sample_rate {
	kHz_8_0 = 0, kHz_5_51, kHz_16_0, kHz_11_025, kHz_27_42, kHz_18_9,
	kHz_32_0, kHz_22_05, kHz_37_8 = 9, kHz_44_1 = 11, kHz_48_0, kHz_33_075,
	kHz_9_6, kHz_6_62
};

enum sample_format {
	linear_8bit_unsigned_mono = 0,		linear_8bit_unsigned_stereo,
	ulaw_8bit_companded_mono,			ulaw_8bit_companded_stereo,		
	linear_16bit_little_endian_mono, 	linear_16bit_little_endian_stereo,
	alaw_8bit_companded_mono,			alaw_8bit_companded_stereo,		
	sample_format_reserved_1,			sample_format_reserved_2,
	adpcm_4bit_mono,					adpcm_4bit_stereo,
	linear_16bit_big_endian_mono,		linear_16bit_big_endian_stereo,
	sample_format_reserved_3,			sample_format_reserved_4
};

struct channel {
	enum adc_source	adc_source;		/* adc input source */
	char		adc_gain;			/* 0..15 adc gain, in 1.5 dB steps */
	char		mic_gain_enable;	/* non-zero enables 20 dB MIC input gain */
	char		aux1_mix_gain;		/* 0..31 aux1 mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	char		aux1_mix_mute;		/* non-zero mutes aux1 mix */
	char		aux2_mix_gain;		/* 0..31 aux2 mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	char		aux2_mix_mute;		/* non-zero mutes aux2 mix */
	char		line_mix_gain;		/* 0..31 line mix to output gain. 12.0 to -34.5 dB in 1.5dB steps */
	char		line_mix_mute;		/* non-zero mutes line mix */
	char		dac_attn;			/* 0..61 dac attenuation, in -1.5 dB steps */
	char		dac_mute;			/* non-zero mutes dac output */
};

typedef struct sound_setup {
	struct channel		left;			/* left channel setup */
	struct channel		right;			/* right channel setup */
	enum sample_rate	sample_rate;	/* sample rate */
	enum sample_format	playback_format;/* sample format for playback */
	enum sample_format	capture_format;	/* sample format for capture */
	char				dither_enable;	/* non-zero enables dither on 16 => 8 bit */
	char				loop_attn;		/* 0..64 adc to dac loopback attenuation, in -1.5 dB steps */
	char				loop_enable;	/* non-zero enables loopback */
	char				output_boost;	/* zero (2.0 Vpp) non-zero (2.8 Vpp) output level boost */
	char				highpass_enable;/* non-zero enables highpass filter in adc */
	char				mono_gain;		/* 0..64 mono speaker gain */
	char				mono_mute;		/* non-zero mutes speaker */
//	char				spdif_mute;		/* non-zero mutes spdif out */
} sound_setup;


/* -----
	control opcodes for sound driver
----- */

enum {
	SOUND_GET_PARAMS = B_DEVICE_OP_CODES_END,
	SOUND_SET_PARAMS,				/* 10000 */
	SOUND_SET_PLAYBACK_COMPLETION_SEM,
	SOUND_SET_CAPTURE_COMPLETION_SEM,
	SOUND_GET_PLAYBACK_TIMESTAMP,	/* 10003 */
	SOUND_GET_CAPTURE_TIMESTAMP,
	SOUND_DEBUG_ON,
	SOUND_DEBUG_OFF,				/* 10006 */
	SOUND_UNSAFE_WRITE,
	SOUND_UNSAFE_READ,
	SOUND_LOCK_FOR_DMA,				/* 10009 */
	SOUND_SET_CAPTURE_PREFERRED_BUF_SIZE,
	SOUND_SET_PLAYBACK_PREFERRED_BUF_SIZE,
	SOUND_GET_CAPTURE_PREFERRED_BUF_SIZE,	/* 10012 */
	SOUND_GET_PLAYBACK_PREFERRED_BUF_SIZE,

// control ports for SPDIF settings
	SOUND_GET_SPDIF_IN_OUT_LOOPBACK,
	SOUND_SET_SPDIF_IN_OUT_LOOPBACK,
	SOUND_GET_SPDIF_OUT,
	SOUND_SET_SPDIF_OUT,
	SOUND_GET_SPDIF_MONITOR,
	SOUND_SET_SPDIF_MONITOR,
	SOUND_GET_SPDIF_OUT_LEVEL,
	SOUND_SET_SPDIF_OUT_LEVEL,
	SOUND_GET_SPDIF_IN_FORMAT,
	SOUND_SET_SPDIF_IN_FORMAT,
	SOUND_GET_SPDIF_IN_OUT_COPYRIGHT,
	SOUND_SET_SPDIF_IN_OUT_COPYRIGHT,
	SOUND_GET_SPDIF_IN_VALIDITY,
	SOUND_SET_SPDIF_IN_VALIDITY,
// control ports for analog settings
	SOUND_GET_4_CHANNEL_DUPLICATE,
	SOUND_SET_4_CHANNEL_DUPLICATE,
// control ports for additional info
	SOUND_GET_DEVICE_ID,
	SOUND_GET_INTERNAL_CHIP_ID,
	SOUND_GET_DRIVER_VERSION
};

#endif
