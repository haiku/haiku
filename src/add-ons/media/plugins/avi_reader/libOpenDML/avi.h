/*
 * Copyright (c) 2004, Marcus Overhagen
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
#ifndef _AVI_H
#define _AVI_H

#include <ByteOrder.h>

#define AVI_UINT32(a)		((uint32)B_LENDIAN_TO_HOST_INT32(a))

#define FOURCC(a,b,c,d)		((((uint32)(d)) << 24) | (((uint32)(c)) << 16) | (((uint32)(b)) << 8) | ((uint32)(a)))
#define FOURCC_FORMAT		"%c%c%c%c"
#define FOURCC_PARAM(f) 	(int)(f & 0xff),(int)((f >> 8) & 0xff),(int)((f >> 16) & 0xff),(int)((f >> 24) & 0xff)

struct avi_main_header
{
	uint32 micro_sec_per_frame;
	uint32 max_bytes_per_sec;
	uint32 padding_granularity;
	uint32 flags;
	uint32 total_frames;
	uint32 initial_frames;
	uint32 streams;
	uint32 suggested_buffer_size;
	uint32 width;
	uint32 height;
	uint32 reserved[4];
} _PACKED;

struct avi_standard_index_entry
{
	uint32 chunk_id;
	uint32 flags;
	uint32 chunk_offset;
	uint32 chunk_length;
} _PACKED;
#define AVIIF_LIST      0x00000001 
#define AVIIF_KEYFRAME  0x00000010 
#define AVIIF_KEYFRAME_SHIFT 4 
#define AVIIF_FIRSTPART 0x00000020 
#define AVIIF_LASTPART  0x00000040 
#define AVIIF_MIDPART   (AVIIF_LASTPART | AVIFF_FIRSTPART) 
#define AVIIF_NOTIME    0x00000100 
#define AVIIF_COMPUSE   0x0fff0000 


struct odml_extended_header
{
	uint32 total_frames;
} _PACKED;

struct avi_stream_header
{
	uint32 fourcc_type;
	uint32 fourcc_handler;
	uint32 flags;
	uint16 priority;
	uint16 language;
	uint32 initial_frames;
	uint32 scale;
	uint32 rate;
	uint32 start;
	uint32 length;
	uint32 suggested_buffer_size;
	uint32 quality;
	uint32 sample_size;
    int16 rect_left;
    int16 rect_top;
    int16 rect_right;
    int16 rect_bottom;
} _PACKED;

struct bitmap_info_header
{
	uint32 size;
	uint32 width;
	uint32 height;
	uint16 planes;
	uint16 bit_count;
	uint32 compression;
	uint32 image_size;
	uint32 x_pels_per_meter;
	uint32 y_pels_per_meter;
	uint32 clr_used;
	uint32 clr_important;
} _PACKED;

struct wave_format_ex
{
	uint16 format_tag;
	uint16 channels;
	uint32 frames_per_sec;
	uint32 avg_bytes_per_sec;
	uint16 block_align;
	uint16 bits_per_sample;
	uint16 extra_size;
	// char extra_data[extra_size]
} _PACKED;

struct odml_index_header
{
	uint16 longs_per_entry;
	uint8 index_sub_type;
	uint8 index_type;
	uint32 entries_used;
	uint32 chunk_id;
	uint32 reserved[3];
	// index entries go here
} _PACKED;

struct odml_chunk_index_header // also field index
{
	uint16 longs_per_entry;
	uint8 index_sub_type;
	uint8 index_type;
	uint32 entries_used;
	uint32 chunk_id;
	uint64 base_offset;
	uint32 reserved;
	// index entries go here
} _PACKED;
typedef odml_chunk_index_header odml_field_index_header;



// index_type codes
#define AVI_INDEX_OF_INDEXES 0x00	// when each entry in aIndex
									// array points to an index chunk
#define AVI_INDEX_OF_CHUNKS  0x01	// when each entry in aIndex array
									// points to a chunk in the file
#define AVI_INDEX_IS_DATA    0x80	// when each entry is aIndex is really the data

// index_sub_type codes for INDEX_OF_CHUNKS
#define AVI_INDEX_2FIELD     0x01	// when fields within frames are also indexed

struct odml_superindex_entry
{
	uint64 start;
	uint32 size;
	uint32 duration;
};

struct odml_index_entry
{
	uint32 start;
	uint32 size; // bit 31 set if not keyframe
};

struct odml_field_index_entry
{
	uint32 start;
	uint32 size; // bit 31 set if not keyframe
	uint32 start_field2;
};

#endif // _AVI_H
