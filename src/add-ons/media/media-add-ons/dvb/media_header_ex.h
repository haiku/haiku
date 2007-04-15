/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __MEDIA_HEADER_EX_H_
#define __MEDIA_HEADER_EX_H_

#include <MediaDefs.h>

struct media_audio_header_ex {
	int32		_reserved_[13];
	float		frame_rate;				// NEW!
	uint32		channel_count;			// NEW!
} _PACKED;


struct media_video_header_ex {
	uint32		_reserved_[8];
	uint32		display_line_width;		// NEW!
	uint32		display_line_count;		// NEW!
	uint32		bytes_per_row;			// NEW!
	uint16		pixel_width_aspect;		// NEW!
	uint16		pixel_height_aspect;	// NEW!
	float		field_gamma;
	uint32		field_sequence;
	uint16		field_number;
	uint16		pulldown_number;
	uint16		first_active_line;
	uint16		line_count;
} _PACKED;


struct media_header_ex {
	media_type						type;
	media_buffer_id 				buffer;
	int32							destination;
	media_node_id					time_source;
	uint32							_reserved1_;
	uint32							size_used;
	bigtime_t						start_time;
	area_id							owner;
	type_code						user_data_type;
	uchar							user_data[64];
	uint32							_reserved2_[2];
	off_t							file_pos;
	size_t							orig_size;
	uint32							data_offset;
	union {
		media_audio_header_ex		raw_audio;
		media_video_header_ex		raw_video;
		media_multistream_header	multistream;
		media_encoded_audio_header	encoded_audio;
		media_encoded_video_header	encoded_video;
		char						_reserved_[64];
	} u;
};


#endif
