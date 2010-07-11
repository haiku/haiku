/*
 * Emuxki BeOS Driver for Creative Labs SBLive!/Audigy series
 *
 * Copyright (c) 2002, Jerome Duval (jerome.duval@free.fr)
 *
 * Authors:
 *		Alexander Coers		Alexander.Coers@gmx.de
 *		Fredrik Modéen 		fredrik@modeen.se
 *
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yannick Montulet.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#if !defined(_EMUXKI_H)
#define _EMUXKI_H

#include <Drivers.h>
#include <SupportDefs.h>
#include <OS.h>
#include "emuxkireg.h"
#include "config.h"
#include "queue.h"
#include "hmulti_audio.h"
#include "multi.h"
#include "joystick_driver.h"


#define CREATIVELABS_VENDOR_ID	0x1102	/* Creative Labs */
#define CREATIVELABS_SBLIVE_DEVICE_ID	0x0002	/* Creative Labs SoundBlaster Live */
#define CREATIVELABS_AUDIGY_DEVICE_ID	0x0004	/* Creative Labs Audigy and Audigy 2 */
#define CREATIVELABS_AUDIGY2_VALUE_DEVICE_ID	0x0008	/* Creative Labs Audigy 2 Value */

#define AUDIGY 1	// Audigy seems to work
#ifdef __HAIKU__
	#define MIDI	1
#else
	#define MIDI	0	// disabled MIDI for the time being on R5
#endif

#define VERSION "Version alpha 7, Copyright (c) 2002,2003 Jérôme Duval, compiled on " __DATE__ " " __TIME__ 
#define DRIVER_NAME "emuxki"
#define FRIENDLY_NAME "Emuxki"
#define FRIENDLY_NAME_LIVE FRIENDLY_NAME" SB Live"
#define FRIENDLY_NAME_LIVE_5_1 FRIENDLY_NAME_LIVE" 5.1"
#define FRIENDLY_NAME_AUDIGY FRIENDLY_NAME" Audigy"
#define FRIENDLY_NAME_AUDIGY2 FRIENDLY_NAME_AUDIGY" 2"
#define FRIENDLY_NAME_AUDIGY2_VALUE FRIENDLY_NAME_AUDIGY2" Value"
#define AUTHOR "Jérôme Duval"

/*
 * Emuxki settings
 */

typedef struct {	
	uint8   channels;	
	uint8   bitsPerSample;	
	uint32  sample_rate;	
	uint32  buffer_frames;	
	int32   buffer_count;	
} emuxki_settings;	
 	 	
extern emuxki_settings current_settings;

/*
 * Gameport stuff
 */

#define HCFG					0x14			/* Hardware Configuration Register of SB-Live */
#define HCFG_JOYENABLE			0x00000200		/* Mask for enabling Joystick */ 

extern generic_gameport_module * gameport;

typedef struct _joy_dev 
{
	void *		driver;
	char		name1[64];
} joy_dev;
/* End Gameport stuff*/

/*
 * Emu10k1 midi
 */

typedef struct _midi_dev {
	struct _emuxki_dev *
				card;
	void *		driver;
	void *		cookie;
	int32		count;
	char		name[64];
} midi_dev;


/*
 * Emu10k1 hardware limits
 */

#define	EMU_PTESIZE		4096
#define	EMU_MAXPTE ((EMU_CHAN_PSST_LOOPSTARTADDR_MASK + 1) /	\
			EMU_PTESIZE)
#define EMU_NUMCHAN		64
#define EMU_NUMRECSRCS	3

#define	EMU_DMA_ALIGN	4096
#define	EMU_DMAMEM_NSEG	1

/*
 * Emu10k1 memory managment
 */

typedef struct _emuxki_mem {
	LIST_ENTRY(_emuxki_mem) next;
	uint16	       ptbidx;
	void	*log_base;
	void	*phy_base;
	area_id area;
	size_t	size;
#define	EMU_RMEM		0xffff		/* recording memory */
} emuxki_mem;

/*
 * Emu10k1 play channel params
 */

typedef struct _emuxki_chanparms_fxsend {
	struct {
		uint8        level, dest;
	}               a, b, c, d, e, f, g, h;
} emuxki_chanparms_fxsend;

typedef struct _emuxki_chanparms_pitch {
	uint16       intial;	/* 4 bits of octave, 12 bits of fractional
				 * octave */
	uint16       current;/* 0x4000 == unity pitch shift */
	uint16       target;	/* 0x4000 == unity pitch shift */
	uint8        envelope_amount;	/* Signed 2's complement, +/-
						 * one octave peak extremes */
} emuxki_chanparms_pitch;

typedef struct _emuxki_chanparms_envelope {
	uint16       current_state;	/* 0x8000-n == 666*n usec delay */
	uint8        hold_time;	/* 127-n == n*(volume ? 88.2 :
					 * 42)msec */
	uint8        attack_time;	/* 0 = infinite, 1 = (volume ? 11 :
					 * 10.9) msec, 0x7f = 5.5msec */
	uint8        sustain_level;	/* 127 = full, 0 = off, 0.75dB
					 * increments */
	uint8        decay_time;	/* 0 = 43.7msec, 1 = 21.8msec, 0x7f =
					 * 22msec */
} emuxki_chanparms_envelope;

typedef struct _emuxki_chanparms_volume {
	uint16       current;
	uint16       target;
	emuxki_chanparms_envelope envelope;
} emuxki_chanparms_volume;

typedef struct _emuxki_chanparms_filter {
	uint16       initial_cutoff_frequency;
	/*
	 * 6 most  significant bits are semitones, 2 least significant bits
	 * are fractions
	 */
	uint16       current_cutoff_frequency;
	uint16       target_cutoff_frequency;
	uint8        lowpass_resonance_height;
	uint8        interpolation_ROM;	/* 1 = full band, 7 = low
						 * pass */
	uint8        envelope_amount;	/* Signed 2's complement, +/-
						 * six octaves peak extremes */
	uint8        LFO_modulation_depth;	/* Signed 2's complement, +/-
						 * three octave extremes */
} emuxki_chanparms_filter;

typedef struct _emuxki_chanparms_loop {
	uint32       start;	/* index in the PTB (in samples) */
	uint32       end;	/* index in the PTB (in samples) */
} emuxki_chanparms_loop;

typedef struct _emuxki_chanparms_modulation {
	emuxki_chanparms_envelope envelope;
	uint16       LFO_state;	/* 0x8000-n = 666*n usec delay */
} emuxki_chanparms_modulation;

typedef struct _emuxki_chanparms_vibrato_LFO {
	uint16       state;	/* 0x8000-n == 666*n usec delay */
	uint8        modulation_depth;	/* Signed 2's complement, +/-
						 * one octave extremes */
	uint8        vibrato_depth;	/* Signed 2's complement, +/- one
					 * octave extremes */
	uint8        frequency;	/* 0.039Hz steps, maximum of 9.85 Hz */
} emuxki_chanparms_vibrato_LFO;

typedef struct _emuxki_channel {
	uint8        num;	/* voice number */
	struct _emuxki_voice *voice;
	emuxki_chanparms_fxsend fxsend;
	emuxki_chanparms_pitch pitch;
	uint16       initial_attenuation;	/* 0.375dB steps */
	emuxki_chanparms_volume volume;
	emuxki_chanparms_filter filter;
	emuxki_chanparms_loop loop;
	emuxki_chanparms_modulation modulation;
	emuxki_chanparms_vibrato_LFO vibrato_LFO;
	uint8        tremolo_depth;
} emuxki_channel;

typedef struct _emuxki_recparams {
	uint32		efx_voices[2];
} emuxki_recparams;


/*
 * Voices
 */

typedef enum {
	EMU_RECSRC_MIC = 0,
	EMU_RECSRC_ADC,
	EMU_RECSRC_FX,
	EMU_RECSRC_NOTSET
} emuxki_recsrc_t;

#define	EMU_USE_PLAY		(1 << 0)
#define	EMU_USE_RECORD	(1 << 1)
#define EMU_STATE_STARTED	(1 << 0)
#define	EMU_STEREO_NOTSET	0xFF

typedef struct _emuxki_voice {
	struct _emuxki_stream 	 *stream;
	uint8        use;
	uint8  		 voicenum;
	uint8        state;
	uint8        stereo;
	uint8        b16;
	uint32       sample_rate;
	union {
		emuxki_channel *chan[2];
		emuxki_recsrc_t source;
	}  			 dataloc;
	emuxki_recparams recparams;
	emuxki_mem *buffer;
	uint16       blksize;/* in samples */
	uint16       trigblk;/* blk on which to trigger inth */
	uint16       blkmod;	/* Modulo value to wrap trigblk */
	uint16       timerate;
	
	LIST_ENTRY(_emuxki_voice) next;
	
} emuxki_voice;

/*
 * Streams
 */

typedef struct _emuxki_stream {
	struct _emuxki_dev 	*card;
	uint8        		use;
	uint8        		state;
	uint8        		stereo;
	uint8        		b16;
	uint32       		sample_rate;
	uint8				nmono;
	uint8				nstereo;
	uint32 				bufframes;
	uint8 				bufcount;
	LIST_HEAD(, _emuxki_voice) voices;
	emuxki_voice		*first_voice;

	LIST_ENTRY(_emuxki_stream)	next;
	
	void            	(*inth) (void *);
	void           		*inthparam;
	
	/* multi_audio */
	volatile int64	frames_count;	// for play or record
	volatile bigtime_t real_time;	// for play or record
	volatile int32	buffer_cycle;	// for play or record
	int32 first_channel;
	bool update_needed;
} emuxki_stream;

/*
 * Mixer controls gpr
 */
#define EMU_GPR_FIRST_MIX 16

typedef enum {
	EMU_MIX_GAIN = 1 << 0,
	EMU_MIX_MONO = 1 << 1,
	EMU_MIX_STEREO = 1 << 2,
	EMU_MIX_TMP = 1 << 3,
	EMU_MIX_MUTE = 1 << 4,
	EMU_MIX_PLAYBACK = 1 << 5,
	EMU_MIX_RECORD = 1 << 6
} emuxki_gpr_type;

typedef struct _emuxki_gpr {
	char name[32];
	emuxki_gpr_type  type;
	uint16	gpr;
	float	default_value;
	float	min_gain;
	float	max_gain;
	float	granularity;
	float 	current[2];
	bool	mute;
} emuxki_gpr;

typedef enum {
	EMU_DIGITAL_MODE = 1 << 0,
	EMU_AUDIO_MODE = 1 << 1
} emuxki_parameter_type;


/*
 * Devices
 */

typedef struct _emuxki_dev {
	char		name[DEVNAME];	/* used for resources */
	pci_info	info;
	device_config config;
	
	void	*ptb_log_base;
	void	*ptb_phy_base;
	area_id ptb_area;
	void	*silentpage_log_base;
	void	*silentpage_phy_base;
	area_id silentpage_area;
	
	emuxki_channel	*channel[EMU_NUMCHAN];
	emuxki_voice	*recsrc[EMU_NUMRECSRCS];
	
	LIST_HEAD(, _emuxki_mem) mem;
	
	LIST_HEAD(, _emuxki_stream) streams;
	
	uint8	timerstate;
	uint16	timerate;
#define	EMU_TIMER_STATE_ENABLED	1
	uint8	play_mode; // number of channels to be played : 2, 4, 6
	bool	digital_enabled;	// if digital is enabled and analog is disabled
	
	emuxki_stream	*pstream;
	emuxki_stream	*pstream2;
	emuxki_stream	*rstream;
	emuxki_stream	*rstream2;
	
	sem_id buffer_ready_sem;
	
	joy_dev		joy;
	midi_dev	midi;
	
	/* mixer controls */
	emuxki_gpr	gpr[256];
	uint32 gpr_count;

	/* multi_audio */
	multi_dev	multi;	
} emuxki_dev;

extern int32 num_cards;
extern emuxki_dev cards[NUM_CARDS];

void emuxki_mem_free(emuxki_dev *card, void *ptr);
void * emuxki_pmem_alloc(emuxki_dev *card, size_t size);
void * emuxki_rmem_alloc(emuxki_dev *card, size_t size);
status_t emuxki_voice_commit_parms(emuxki_voice *voice);
status_t emuxki_voice_set_audioparms(emuxki_voice *voice, uint8 stereo,
			     uint8 b16, uint32 srate);
status_t emuxki_voice_set_recparms(emuxki_voice *voice, emuxki_recsrc_t recsrc,
			     	emuxki_recparams *recparams);
status_t emuxki_voice_set_bufparms(emuxki_voice *voice, void *ptr,
			   uint32 bufsize, uint16 blksize);
void emuxki_voice_start(emuxki_voice *voice);
void emuxki_voice_halt(emuxki_voice *voice);
emuxki_voice *emuxki_voice_new(emuxki_stream *stream, uint8 use, uint8 voicenum);
void emuxki_voice_delete(emuxki_voice *voice);

status_t emuxki_stream_set_audioparms(emuxki_stream *stream, bool stereo, uint8 channels,
			     uint8 b16, uint32 sample_rate);
status_t emuxki_stream_set_recparms(emuxki_stream *stream, emuxki_recsrc_t recsrc,
			     	emuxki_recparams *recparams);
status_t emuxki_stream_commit_parms(emuxki_stream *stream);
status_t emuxki_stream_get_nth_buffer(emuxki_stream *stream, uint8 chan, uint8 buf, 
					char** buffer, size_t *stride);
void emuxki_stream_start(emuxki_stream *stream, void (*inth) (void *), void *inthparam);
void emuxki_stream_halt(emuxki_stream *stream);
emuxki_stream *emuxki_stream_new(emuxki_dev *card, uint8 use, uint32 bufframes, uint8 bufcount);
void emuxki_stream_delete(emuxki_stream *stream);

void emuxki_dump_fx(emuxki_dev * card);
void emuxki_gpr_dump(emuxki_dev * card, uint16 count);
void emuxki_gpr_set(emuxki_dev *card, emuxki_gpr *gpr, int32 type, float *values);
void emuxki_gpr_get(emuxki_dev *card, emuxki_gpr *gpr, int32 type, float *values);

void emuxki_parameter_set(emuxki_dev *card, const void*, int32 type, int32 *value);
void emuxki_parameter_get(emuxki_dev *card, const void*, int32 type, int32 *value);

extern void midi_interrupt_op(int32 op, void * data);
extern bool midi_interrupt(emuxki_dev *card);

#endif

