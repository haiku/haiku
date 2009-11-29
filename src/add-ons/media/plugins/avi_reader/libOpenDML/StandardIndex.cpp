/*
 * Copyright (c) 2008, Stephan Aßmus <superstippi@gmx.de>
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
#include <DataIO.h>
#include <math.h>
#include <stdio.h>
#include <new>

#include "StandardIndex.h"


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


StandardIndex::StandardIndex(BPositionIO *source, OpenDMLParser *parser)
 :	Index(source, parser)
 ,	fIndex(NULL)
 ,	fIndexSize(0)
 ,	fDataOffset(parser->MovieListStart() - 4)
{
}


StandardIndex::~StandardIndex()
{
	delete [] fIndex;
}


status_t
StandardIndex::Init()
{
	TRACE("Constructing a Standard Index\n");
	uint32 indexBytes = fParser->StandardIndexSize();
	fIndexSize = indexBytes / sizeof(avi_standard_index_entry);
	indexBytes = fIndexSize * sizeof(avi_standard_index_entry);
	
	if (indexBytes > 0x1900000) { // 25 MB
		ERROR("libOpenDML: StandardIndex::Init index is way too big\n");
		return B_NO_MEMORY;
	}

	if (fIndexSize == 0) {
		printf("StandardIndex::Init index is empty\n");
		return B_NO_MEMORY;
	}

	fIndex = new (std::nothrow) avi_standard_index_entry[fIndexSize];
	if (fIndex == NULL) {
		ERROR("libOpenDML: StandardIndex::Init out of memory\n");
		return B_NO_MEMORY;
	}

	if ((int32)indexBytes != fSource->ReadAt(fParser->StandardIndexStart(),
			fIndex, indexBytes)) {
		ERROR("libOpenDML: StandardIndex::Init file reading failed\n");
		delete [] fIndex;
		fIndex = NULL;
		return B_IO_ERROR;
	}

	int stream_index;
	int64 position;
	uint32 size;
	uint64 frame[fStreamCount];
	uint64 frame_no;
	bigtime_t pts = 0;
	bool keyframe;
	uint32 sample_size;
	
	const OpenDMLStream *stream;

	for (uint32 i=0;i < fStreamCount; i++) {
		frame[i] = 0;
	}

	for (uint32 i = 0; i < fIndexSize; i++) {
		
		if (fIndex[i].chunk_id == 0) {
			printf("Corrupt Index entry at %ld/%ld\n",i,fIndexSize);
			continue;
		}
				
		stream_index = ((fIndex[i].chunk_id & 0xff) - '0') * 10;
		stream_index += ((fIndex[i].chunk_id >> 8) & 0xff) - '0';

		stream = fParser->StreamInfo(stream_index);
        
		if (stream == NULL) {
			printf("No Stream Info for stream %d\n",stream_index);
			continue;
		}
        
        keyframe = (fIndex[i].flags >> AVIIF_KEYFRAME_SHIFT) & 1;
        size = fIndex[i].chunk_length;
        
        // Some muxers write chunk_offset as non-relative.  So we test if the first index actually points to a chunk
        if (i == 0) {
        	off_t here = fSource->Position();

        	// first try and validate position as relative to the data chunk
	        position = fDataOffset + fIndex[i].chunk_offset;
	        TRACE("Validating chunk position %Ld as relative\n",position);
			if (!IsValidChunk(position,size)) {
			
	        	// then try and validate position as an absolute position that points to a chunk
				fDataOffset = 0;
		        position = fDataOffset + fIndex[i].chunk_offset;
		        TRACE("Validating chunk position %Ld as absolute\n",position);
				if (!IsValidChunk(position,size)) {
					ERROR("Index is invalid, chunk offset does not point to a chunk\n");
					return B_ERROR;
				}
			}
			
			fSource->Seek(here, SEEK_SET);
        }
        
        position = fDataOffset + fIndex[i].chunk_offset;
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
					// ABR Audio so use Chunk Size and average bytes per second to determine how many samples there are in the chunk
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
	}

	return B_OK;
}

bool
StandardIndex::IsValidChunk(off_t position, uint32 size) {

	chunk aChunk;

	if ((int32)8 != fSource->ReadAt(position, &aChunk, 8)) {
		return false;
	}

	return (aChunk.size == size);
}
