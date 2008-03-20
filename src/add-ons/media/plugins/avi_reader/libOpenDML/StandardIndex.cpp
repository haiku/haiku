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
#include <DataIO.h>
#include <stdio.h>
#include <new>
#include "ReaderPlugin.h" // B_MEDIA_*
#include "StandardIndex.h"
#include "OpenDMLParser.h"

#include <StopWatch.h>


//#define TRACE_START_INDEX
#ifdef TRACE_START_INDEX
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)


StandardIndex::StandardIndex(BPositionIO *source, OpenDMLParser *parser)
 :	Index(source, parser)
 ,	fIndex(NULL)
 ,	fIndexSize(0)
 ,	fStreamData(NULL)
 ,	fStreamCount(parser->StreamCount())
 ,	fDataOffset(parser->MovieListStart() - 4)
{
}


StandardIndex::~StandardIndex()
{
	delete [] fIndex;
	if (fStreamData) {
		for (int i = 0; i < fStreamCount; i++)
			delete [] fStreamData[i].seek_hints;
	}
	delete [] fStreamData;
}


status_t
StandardIndex::Init()
{
	uint32 indexBytes = fParser->StandardIndexSize();
	fIndexSize = indexBytes / sizeof(avi_standard_index_entry);
	indexBytes = fIndexSize * sizeof(avi_standard_index_entry);
	uint32 seekHintsStride = 1800 * fStreamCount;
	uint32 seekHintsMax = fIndexSize / seekHintsStride;
	
	TRACE("StandardIndex::Init: seekHintsStride %lu\n", seekHintsStride);
	TRACE("StandardIndex::Init: seekHintsMax %lu\n", seekHintsMax);

#ifdef TRACE_START_INDEX
{ BStopWatch w("StandardIndex::Init: malloc");
#endif

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
#ifdef TRACE_START_INDEX
}
{ BStopWatch w("StandardIndex::Init: file read");
#endif

	if ((int32)indexBytes != fSource->ReadAt(fParser->StandardIndexStart(),
			fIndex, indexBytes)) {
		ERROR("libOpenDML: StandardIndex::Init file reading failed\n");
		delete [] fIndex;
		fIndex = NULL;
		return B_IO_ERROR;
	}
#ifdef TRACE_START_INDEX
}
	//DumpIndex();
#endif

	fStreamData = new stream_data[fStreamCount];
	for (int i = 0; i < fStreamCount; i++) {
		fStreamData[i].chunk_id = i / 10 + '0' + (i % 10 + '0') * 256;
		fStreamData[i].chunk_count = 0;
		fStreamData[i].keyframe_count = 0;
		fStreamData[i].stream_pos = 0;
		fStreamData[i].stream_size = 0;
		fStreamData[i].seek_hints = new seek_hint[seekHintsMax];
		fStreamData[i].seek_hints_count = 0;
		fStreamData[i].seek_hints_next = seekHintsStride;
	}

#ifdef TRACE_START_INDEX
{ BStopWatch w("StandardIndex::Init: scan index");
#endif
	
	for (int stream = 0; stream < fStreamCount; stream++) {
		uint32 chunk_id = fStreamData[stream].chunk_id;
		uint64 stream_size = 0;
		uint32 chunk_count = 0;
		uint32 keyframe_count = 0;
		uint32 seek_hints_next = seekHintsStride;
		uint32 seek_hints_count = 0;
		for (uint32 i = 0; i < fIndexSize; i++) {
			if ((fIndex[i].chunk_id & 0xffff) == chunk_id) {
				stream_size += fIndex[i].chunk_length;
				chunk_count++;
				keyframe_count += (fIndex[i].flags >> AVIIF_KEYFRAME_SHIFT) & 1;

				if (i >= seek_hints_next) {
					seek_hints_next = i + seekHintsStride;
					seek_hint *hint = &fStreamData[stream].seek_hints[
						seek_hints_count++];
					hint->stream_pos = fIndex[i].chunk_offset;
					hint->index_pos = i;
				}
			}
		}
		fStreamData[stream].stream_size = stream_size;
		fStreamData[stream].chunk_count = chunk_count;
		fStreamData[stream].keyframe_count = keyframe_count;
		fStreamData[stream].seek_hints_count = seek_hints_count;
	}
#ifdef TRACE_START_INDEX
}

	for (int i = 0; i < fStreamCount; i++) {
		printf("stream %d, stream_size %llu\n", i, fStreamData[i].stream_size);
		printf("stream %d, chunk_count %lu\n", i, fStreamData[i].chunk_count);
		printf("stream %d, keyframe_count %lu\n", i,
			fStreamData[i].keyframe_count);
		printf("stream %d, seek_hints_count %lu\n", i,
			fStreamData[i].seek_hints_count);
		for (int j = 0; j < fStreamData[i].seek_hints_count; j++) {
			printf("  seek_hint %3d, index_pos %6lu, stream_pos %lld\n", j,
				fStreamData[i].seek_hints[j].index_pos, 
				fStreamData[i].seek_hints[j].stream_pos);
		}
	}
#endif // TRACE_START_INDEX

	return B_OK;
}


void 
StandardIndex::DumpIndex()
{
	uint32 chunk = fIndex->chunk_id;
	int count = 0;
	int pos = 0;

	printf("StandardIndex::DumpIndex %lu entries\n", fIndexSize);
	for (uint32 i = 0; i < fIndexSize; i++) {
		count++;
		if (chunk != fIndex[i].chunk_id) {
			printf("%3d %c%c%c%c", count, FOURCC_PARAM(chunk));
			chunk = fIndex[i].chunk_id;
			count = 0;
			if (++pos % 8 == 0)
				printf("\n");			
		}
	}
	if (count)
		printf("%3d %c%c%c%c", count, FOURCC_PARAM(chunk));
	printf("\n");			
}


status_t
StandardIndex::GetNextChunkInfo(int stream_index, int64 *start, uint32 *size,
	bool *keyframe)
{
	stream_data *data = &fStreamData[stream_index];
	while (data->stream_pos < fIndexSize) {
		if ((fIndex[data->stream_pos].chunk_id & 0xffff) == data->chunk_id) {
			*keyframe = fIndex[data->stream_pos].flags & AVIIF_KEYFRAME;
			*start = fDataOffset + fIndex[data->stream_pos].chunk_offset + 8; 
				// skip 8 bytes (chunk id + chunk size)
			*size = fIndex[data->stream_pos].chunk_length;
			data->stream_pos++;
			return B_OK;
		}
		data->stream_pos++;
	}

	return B_ERROR;
}


status_t
StandardIndex::Seek(int stream_index, uint32 seekTo, int64 *frame,
	bigtime_t *time, bool readOnly)
{
	TRACE("StandardIndex::Seek: stream %d, seekTo%s%s%s%s, time %Ld, "
		"frame %Ld\n", stream_index,
		(seekTo & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(seekTo & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(seekTo & B_MEDIA_SEEK_CLOSEST_FORWARD) ?
			" B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		(seekTo & B_MEDIA_SEEK_CLOSEST_BACKWARD) ?
			" B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		*time, *frame);
	
	const stream_info *stream = fParser->StreamInfo(stream_index);
	stream_data *data = &fStreamData[stream_index];

	int64 frame_pos;
	if (seekTo & B_MEDIA_SEEK_TO_FRAME)
		frame_pos = *frame;
	else if (seekTo & B_MEDIA_SEEK_TO_TIME) {
		frame_pos = (*time * stream->frames_per_sec_rate)
			/ (1000000 * stream->frames_per_sec_scale);
	} else
		return B_BAD_VALUE;

	// TODO: Actually take keyframe flags into account!
	if (stream->is_audio) {
		int64 bytes = 0;
		for (uint32 i = 0; i < fIndexSize; i++) {
			if ((fIndex[i].chunk_id & 0xffff) == data->chunk_id) {
				int64 bytesNext = bytes + fIndex[i].chunk_length;
				if (bytes <= frame_pos && bytesNext > frame_pos) {
					if (!readOnly)
						data->stream_pos = i;
					goto done;
				}
				bytes = bytesNext;
			}
		}
	} else if (stream->is_video) {
		int pos = 0;
		for (uint32 i = 0; i < fIndexSize; i++) {
			if ((fIndex[i].chunk_id & 0xffff) == data->chunk_id) {
				if (pos == frame_pos) {
					if (!readOnly)
						data->stream_pos = i;
					goto done;
				}
				pos++;
			}
		}
	} else {
		return B_BAD_VALUE;
	}

	ERROR("libOpenDML: seek failed, position not found\n");
	return B_ERROR;

done:
	TRACE("seek done: index: pos %d, size  %d\n", data->stream_pos, fIndexSize);
	*frame = frame_pos;
	*time = (frame_pos * 1000000 * stream->frames_per_sec_scale)
		/ stream->frames_per_sec_rate;
	return B_OK;
}

