/*
 * Copyright (c) 2003-2004, Marcus Overhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _AU_H
#define _AU_H

struct snd_header { 
	uint32 magic;			// magic number '.snd'
	uint32 data_start;		// offset to the data
	uint32 data_size;		// number of bytes of data
	uint32 data_format;		// data format code
	uint32 sampling_rate;	// sampling rate
	uint32 channel_count;	// number of channels
	char info[4];			// optional text information, NULL termintated, at least 4 bytes
};

enum {
	SND_FORMAT_UNSPECIFIED = 0,	// unspecified format
	SND_FORMAT_MULAW_8 = 1,		// 8-bit mu-law samples 
	SND_FORMAT_LINEAR_8 = 2,	// 8-bit linear samples 
	SND_FORMAT_LINEAR_16 = 3,	// 16-bit linear samples 
	SND_FORMAT_LINEAR_24 = 4,	// 24-bit linear samples 
	SND_FORMAT_LINEAR_32 = 5,	// 32-bit linear samples 
	SND_FORMAT_FLOAT = 6,		// 32-bit IEEE floating-point samples 
	SND_FORMAT_DOUBLE = 7,		// 64-bit IEEE floating-point samples 
	SND_FORMAT_INDIRECT = 8,	// fragmented sampled data 
	SND_FORMAT_NESTED = 9,
	SND_FORMAT_DSP_CORE = 10,	// DSP program 
	SND_FORMAT_DSP_DATA_8 = 11,	// 8-bit fixed-point samples 
	SND_FORMAT_DSP_DATA_16 = 12,// 16-bit fixed-point samples 
	SND_FORMAT_DSP_DATA_24 = 13,// 24-bit fixed-point samples 
	SND_FORMAT_DSP_DATA_32 = 14,// 32-bit fixed-point samples 
	SND_FORMAT_15 = 15,
	SND_FORMAT_DISPLAY = 16,	// non-audio display data 
	SND_FORMAT_MULAW_SQUELCH = 17,
	SND_FORMAT_EMPHASIZED = 18,	// 16-bit linear with emphasis 
	SND_FORMAT_COMPRESSED = 19,	// 16-bit linear with compression 
	SND_FORMAT_COMPRESSED_EMPHASIZED = 20, // A combination of the two above 
	SND_FORMAT_DSP_COMMANDS = 21,// Music Kit DSP commands 
	SND_FORMAT_DSP_COMMANDS_SAMPLES = 22,
	SND_FORMAT_ADPCM_G721 = 23,
	SND_FORMAT_ADPCM_G722 = 24,
	SND_FORMAT_ADPCM_G723_3 = 25,
	SND_FORMAT_ADPCM_G723_5 = 26,
	SND_FORMAT_ALAW_8 = 27,
};

#define SND_RATE_8012	8012.821	// CODEC input
#define SND_RATE_22050	22050.0		// low sampling rate output
#define SND_RATE_44100	44100.0		// high sampling rate output

#define SND_MAGIC 0x2e736e64		// '.snd'

#endif
