/*
 * Copyright (c) 2007, Marcus Overhagen
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
#include <SupportDefs.h>
#include <math.h>
#include <stdio.h>
#include "FallbackIndex.h"

//#define TRACE_START_INDEX
#ifdef TRACE_START_INDEX
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)

struct chunk {
	uint32 chunk_id;
	uint32 size;
};

FallbackIndex::FallbackIndex(BPositionIO *source, OpenDMLParser *parser)
 :	Index(source, parser)
{
}


FallbackIndex::~FallbackIndex()
{
}


status_t
FallbackIndex::Init()
{
	// Attempt to build an index by parsing the movi chunk
	TRACE("Constructing a Fallback Index\n");

	bool end_of_movi = false;
	chunk aChunk;

	int stream_index;
	off_t position;
	uint32 size;
	uint64 frame[fStreamCount];
	uint64 frame_no;
	bigtime_t pts = 0;
	bool keyframe = false;
	uint32 sample_size;
	uint64 entries = 0;
	
	const OpenDMLStream *stream;

	for (uint32 i=0;i < fStreamCount; i++) {
		frame[i] = 0;
	}

	position = fParser->MovieListStart();

	while (end_of_movi == false) {

		if ((int32)8 != fSource->ReadAt(position, &aChunk, 8)) {
			ERROR("libOpenDML: FallbackIndex::Init file reading failed\n");
			return B_IO_ERROR;
		}

		position += 8;

		stream_index = ((aChunk.chunk_id & 0xff) - '0') * 10;
        stream_index += ((aChunk.chunk_id >> 8) & 0xff) - '0';

		if ((stream_index < 0) || (stream_index >= fStreamCount)) {
			if (entries == 0) {
				ERROR("libOpenDML: FallbackIndex::Init - Failed to build an index, file is too corrupt\n");
				return B_IO_ERROR;
			} else {
				ERROR("libOpenDML: FallbackIndex::Init - Error while trying to build index, file is corrupt but will continue after creating %Ld entries in index\n",entries);
				return B_OK;
			}
		}

		entries++;

		stream = fParser->StreamInfo(stream_index);

		size = aChunk.size;
		frame_no = frame[stream_index];

		if (stream->is_video) {
			// Video is easy enough, it is always 1 frame = 1 index
			pts = frame[stream_index] * 1000000LL * stream->frames_per_sec_scale / stream->frames_per_sec_rate;
			frame[stream_index]++;
		} else if (stream->is_audio) {
			pts = frame[stream_index] * 1000000LL / stream->audio_format->frames_per_sec;

			// Audio varies based on many different hacks over the years
			// The simplest is chunk size / sample size = no of samples in the chunk for uncompressed audio
			// ABR Compressed audio is more difficult and VBR Compressed audio is even harder
			// What follows is what I have worked out from various sources across the internet.
			if (stream->audio_format->format_tag != 0x0001) {
				// VBR audio is detected as having a block_align >= 960
				if (stream->audio_format->block_align >= 960) {
					// VBR Audio so block_align is the largest no of samples in a chunk
					// scale is the no of samples in a frame
					// rate is the sample rate
					// but we must round up when calculating no of frames in a chunk
					frame[stream_index] += (uint64)(ceil((double)size / (double)stream->audio_format->block_align)) * stream->frames_per_sec_scale;
				} else {
					// ABR Audio so use Chunk Size and avergae bytes per second to determine how many samples there are in the chunk
					frame[stream_index] += (uint64)(ceil((double)size / (double)stream->audio_format->block_align)) * stream->audio_format->frames_per_sec / stream->audio_format->avg_bytes_per_sec;
				}
			} else {
				// sample size can be corrupt
				if (stream->stream_header.sample_size > 0) {
					sample_size = stream->stream_header.sample_size;
				} else {
					// compute sample size
					sample_size = stream->audio_format->bits_per_sample * stream->audio_format->channels / 8;
				}
				frame[stream_index] += size / stream->stream_header.sample_size;
			}
		}

		AddIndex(stream_index, position, size, frame_no, pts, keyframe);
		
		position += aChunk.size;
		
		end_of_movi = position >= fParser->MovieListSize();
	}
	
	return B_OK;
}
