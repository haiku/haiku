/* audio_driver.h */
/* Jon Watte 19971223 */
/* Interface to drivers found in /dev/audio */
/* Devices found in /dev/audio/old follow a different API! */

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#if !defined(_AUDIO_DRIVER_H)
#define _AUDIO_DRIVER_H

#if !defined(_SUPPORT_DEFS_H)
#include <SupportDefs.h>
#endif /* _SUPPORT_DEFS_H */
#if !defined(_DRIVERS_H)
#include <Drivers.h>
#endif /* _DRIVERS_H */

enum {
	/* arg = ptr to struct audio_format */
	B_AUDIO_GET_AUDIO_FORMAT = B_AUDIO_DRIVER_BASE,
	B_AUDIO_SET_AUDIO_FORMAT,
	/* arg = ptr to float[4] */
	B_AUDIO_GET_PREFERRED_SAMPLE_RATES,
	/* arg = ptr to struct audio_routing_cmd */
	B_ROUTING_GET_VALUES,
	B_ROUTING_SET_VALUES,
	/* arg = ptr to struct audio_routing_cmd */
	B_MIXER_GET_VALUES,
	B_MIXER_SET_VALUES,
	/* arg = ptr to struct audio_timing */
	B_AUDIO_GET_TIMING		/* used to be SV_SECRET_HANDSHAKE (10100) */
};

/* this is the definition of what the audio driver can do for you */
typedef struct audio_format {
    float       sample_rate;    /* ~4000 - ~48000, maybe more */
    int32       channels;       /* 1 or 2 */
    int32       format;         /* 0x11 (uchar), 0x2 (short) or 0x24 (float) */
    int32       big_endian;    /* 0 for little endian, 1 for big endian */
    size_t      buf_header;     /* typically 0 or 16 */
	size_t      play_buf_size;	/* size of playback buffer (latency) */
	size_t		rec_buf_size;	/* size of record buffer (latency) */
} audio_format;

/* when buffer header is in effect, this is what gets read before data */
typedef struct audio_buf_header {
    bigtime_t   capture_time;
    uint32      capture_size;
    float       sample_rate;
} audio_buf_header;

/* the mux devices use these records */
typedef struct audio_routing {
	int32 selector;	/* for GET, these need to be filled in! */
	int32 value;
} audio_routing;

/* this is the argument for ioctl() */
typedef struct audio_routing_cmd {
	int32 count;
	audio_routing* data;
} audio_routing_cmd;

typedef struct {
    bigtime_t   wr_time;
	bigtime_t   rd_time;
	uint32      wr_skipped;
	uint32      rd_skipped;
	uint64      wr_total;
	uint64      rd_total;
	uint32      _reserved_[6];
} audio_timing;


enum {	/* selectors for routing */
	B_AUDIO_INPUT_SELECT,
	B_AUDIO_MIC_BOOST,
	B_AUDIO_MIDI_OUTPUT_TO_SYNTH,
	B_AUDIO_MIDI_INPUT_TO_SYNTH,
	B_AUDIO_MIDI_OUTPUT_TO_PORT,
	B_AUDIO_PCM_OUT_POST_3D,
	B_AUDIO_STEREO_ENHANCEMENT,
	B_AUDIO_3D_STEREO_ENHANCEMENT,
	B_AUDIO_BASS_BOOST,
	B_AUDIO_LOCAL_LOOPBACK,
	B_AUDIO_REMOTE_LOOPBACK,
	B_AUDIO_MONO_OUTPUT_FROM_MIC,
	B_AUDIO_ALTERNATE_MIC,
	B_AUDIO_ADC_DAC_LOOPBACK
};

enum {	/* input MUX source values */
	B_AUDIO_INPUT_DAC = 1,	/* (not in AC97) */
	B_AUDIO_INPUT_LINE_IN,
	B_AUDIO_INPUT_CD,
	B_AUDIO_INPUT_VIDEO,
	B_AUDIO_INPUT_AUX1,		/* synth */
	B_AUDIO_INPUT_AUX2,		/* (not in AC97) */
	B_AUDIO_INPUT_PHONE,
	B_AUDIO_INPUT_MIC,
	B_AUDIO_INPUT_MIX_OUT,	/* sum of all outputs */
	B_AUDIO_INPUT_MONO_OUT
};

/* the mixer devices use these records */
typedef struct audio_level {
	int32 selector;	/* for GET, this still needs to be filled in! */
	uint32 flags;
	float value;	/* in dB */
} audio_level;

/* this is the arg to ioctl() */
typedef struct audio_level_cmd {
	int32 count;
	audio_level* data;
} audio_level_cmd;

/* bitmask for the flags */
#define B_AUDIO_LEVEL_MUTED 1	

enum {	/* selectors for levels */
  B_AUDIO_MIX_ADC_LEFT,
  B_AUDIO_MIX_ADC_RIGHT,
  B_AUDIO_MIX_DAC_LEFT,
  B_AUDIO_MIX_DAC_RIGHT,
  B_AUDIO_MIX_LINE_IN_LEFT,
  B_AUDIO_MIX_LINE_IN_RIGHT,
  B_AUDIO_MIX_CD_LEFT,
  B_AUDIO_MIX_CD_RIGHT,
  B_AUDIO_MIX_VIDEO_LEFT,
  B_AUDIO_MIX_VIDEO_RIGHT,
  B_AUDIO_MIX_SYNTH_LEFT,
  B_AUDIO_MIX_SYNTH_RIGHT,
  B_AUDIO_MIX_AUX_LEFT,			/* (not in AC97) */
  B_AUDIO_MIX_AUX_RIGHT,		/* (not in AC97) */
  B_AUDIO_MIX_PC_BEEP,
  B_AUDIO_MIX_PHONE,
  B_AUDIO_MIX_MIC,
  B_AUDIO_MIX_LINE_OUT_LEFT,
  B_AUDIO_MIX_LINE_OUT_RIGHT,
  B_AUDIO_MIX_HEADPHONE_OUT_LEFT,
  B_AUDIO_MIX_HEADPHONE_OUT_RIGHT,
  B_AUDIO_MIX_MONO_OUT,
  B_AUDIO_MIX_LOOPBACK_LEVEL	/* (not in AC97) */
};

#endif /* _AUDIO_DRIVER_H */

