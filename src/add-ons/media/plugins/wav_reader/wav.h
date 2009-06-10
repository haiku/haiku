/*
 * Copyright (c) 2003, Marcus Overhagen
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
#ifndef _WAV_H
#define _WAV_H

struct riff_struct
{ 
	uint32 riff_id; // 'RIFF'
	uint32 len;
	uint32 wave_id;	// 'WAVE'
}; 

struct chunk_struct 
{ 
	uint32 fourcc;
	uint32 len;
};

struct format_struct 
{ 
	uint16 format_tag; 
	uint16 channels; 
	uint32 samples_per_sec; 
	uint32 avg_bytes_per_sec; 
	uint16 block_align; 
	uint16 bits_per_sample;
}; 

struct wave_format_ex {
	uint16 format_tag;
	uint16 channels;
	uint32 samples_per_sec;
	uint32 avg_bytes_per_sec;
	uint16 block_align;
	uint16 bits_per_sample;
	uint16 extra_size;
	uint8  extra_data[64];  // allow for 64 bytes of extra data
};

struct mpeg3_wav_format {
	uint16 format_tag;
	uint16 channels;
	uint32 samples_per_sec;
	uint32 avg_bytes_per_sec;
	uint16 block_align;
	uint16 bits_per_sample;
	uint16 extra_size;
	uint16 id;
	uint32 flags;
	uint16 block_size;
	uint16 frames_per_block;
	uint16 codec_delay;
};

struct mpeg1_wav_format {
	uint16 format_tag;
	uint16 channels;
	uint32 samples_per_sec;
	uint32 avg_bytes_per_sec;
	uint16 block_align;
	uint16 bits_per_sample;
	uint16 extra_size;
	uint16 head_layer;
	uint32 head_bitrate;
	uint16 head_mode;
	uint16 head_mode_ext;
	uint16 head_emphasis;
	uint16 head_flags;
	uint32 pts_low;
	uint32 pts_high;
};

struct format_struct_extensible
{ 
	uint16 format_tag; // 0xfffe for extensible format
	uint16 channels; 
	uint32 samples_per_sec; 
	uint32 avg_bytes_per_sec; 
	uint16 block_align; 
	uint16 bits_per_sample;
	uint16 ext_size;
	uint16 valid_bits_per_sample;
	uint32 channel_mask;
	uint8  guid[16]; // first two bytes are format code
}; 

struct fact_struct 
{ 
	uint32 sample_length; 
}; 

#endif
