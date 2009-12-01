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

#include "OpenDMLIndex.h"

//#define TRACE_START_INDEX
#ifdef TRACE_START_INDEX
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)


OpenDMLIndex::OpenDMLIndex(BPositionIO *source, OpenDMLParser *parser)
	:
	Index(source, parser)
{
}


OpenDMLIndex::~OpenDMLIndex()
{
}


status_t
OpenDMLIndex::Init()
{
	TRACE("Constructing a OpenDML Index\n");

	uint32 lastFrame;
	bigtime_t pts;
	odml_index_header superIndex;
	const OpenDMLStream *stream;

	int64 pos = 0;
	uint32 bytesRead;

	for (int32 i = 0; i < fStreamCount; i++) {
		stream = fParser->StreamInfo(i);

		pos = stream->odml_index_start;
		bytesRead = sizeof(odml_index_header);

		if ((int32)bytesRead != fSource->ReadAt(pos, &superIndex, bytesRead)) {
			ERROR("libOpenDML: OpenDMLIndex::Init file reading failed to read "
				"superindex\n");
			return B_IO_ERROR;
		}

		pos += bytesRead;

		pts = 0;
		lastFrame = 0;

		ReadIndex(i, pos, &lastFrame, &pts, &superIndex);
	}

	return B_OK;
}


status_t
OpenDMLIndex::ReadIndex(uint32 stream_index, int64 pos, uint32 *lastFrame,
	bigtime_t *pts, odml_index_header *superIndex)
{
	odml_superindex_entry superIndexEntry;
	const OpenDMLStream *stream = fParser->StreamInfo(stream_index);

	for (uint32 i = 0; i < superIndex->entries_used; i++) {
		if (superIndex->index_type == AVI_INDEX_OF_INDEXES) {
			ssize_t bytesRead = fSource->ReadAt(pos, &superIndexEntry,
				sizeof(odml_superindex_entry));
			if (bytesRead < 0)
				return bytesRead;
			if (bytesRead != (ssize_t)sizeof(odml_superindex_entry)) {
				ERROR("libOpenDML: OpenDMLIndex::Init file reading failed to "
					"read superindex entry\n");
				return B_IO_ERROR;
			}

			pos += bytesRead;

			if (superIndexEntry.start == 0)
				continue;

			if (superIndex->index_sub_type != AVI_INDEX_2FIELD) {
				ReadChunkIndex(stream_index, superIndexEntry.start + 8,
					lastFrame, pts, stream);
			}
		}
	}

	return B_OK;
}


status_t
OpenDMLIndex::ReadChunkIndex(uint32 stream_index, int64 pos, uint32 *lastFrame,
	bigtime_t *pts, const OpenDMLStream *stream)
{
	odml_index_entry chunkIndexEntry;
	odml_chunk_index_header chunkIndex;
	uint32 bytesRead;
	uint32 frameNo;
	bool keyframe;
	uint32 sample_size;
	int64 position;
	uint32 size;

	bytesRead = sizeof(odml_chunk_index_header);

	if ((int32)bytesRead != fSource->ReadAt(pos, &chunkIndex, bytesRead)) {
		ERROR("libOpenDML: OpenDMLIndex::Init file reading failed to read Chunk Index\n");
		return B_IO_ERROR;
	}

	pos += bytesRead;

	for (uint32 i=0; i<chunkIndex.entries_used; i++) {
		bytesRead = sizeof(odml_index_entry);

		if ((int32)bytesRead != fSource->ReadAt(pos, &chunkIndexEntry, bytesRead)) {
			ERROR("libOpenDML: OpenDMLIndex::Init file reading failed to read Chunk Index Entry\n");
			return B_IO_ERROR;
		}

		pos += bytesRead;

		keyframe = (chunkIndexEntry.size & 0x80000000) ? false : true;
		position = chunkIndex.base_offset + chunkIndexEntry.start - 8;
		size = chunkIndexEntry.size & 0x7fffffff;
		frameNo = *lastFrame;

		if (stream->is_video) {
			// Video is easy enough, it is always 1 frame = 1 index
			*pts = *lastFrame * 1000000LL * stream->frames_per_sec_scale / stream->frames_per_sec_rate;
			(*lastFrame)++;
		} else if (stream->is_audio) {

			*pts = *lastFrame * 1000000LL / stream->audio_format->frames_per_sec;

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
					*lastFrame += (uint64)(ceil((double)size / (double)stream->audio_format->block_align)) * stream->frames_per_sec_scale;
				} else {
					// ABR Audio so use Chunk Size and average bytes per second to determine how many samples there are in the chunk
					*lastFrame += (uint64)(ceil((double)size / (double)stream->audio_format->block_align)) * stream->audio_format->frames_per_sec / stream->audio_format->avg_bytes_per_sec;
				}
			} else {
				// sample size can be corrupt
				if (stream->stream_header.sample_size > 0) {
					sample_size = stream->stream_header.sample_size;
				} else {
					// compute sample size
					sample_size = stream->audio_format->bits_per_sample * stream->audio_format->channels / 8;
				}
				*lastFrame += size / stream->stream_header.sample_size;
			}
		}

		AddIndex(stream_index, position, size, frameNo, *pts, keyframe);
	}

	return B_OK;
}

