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
#include <stdio.h>
#include "ReaderPlugin.h" // B_MEDIA_*
#include "Index.h"

//#define TRACE_INDEX
#ifdef TRACE_INDEX
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)

Index::Index(BPositionIO *source, OpenDMLParser *parser)
 :	fSource(source)
 ,	fParser(parser)
{
	fStreamCount = parser->StreamCount();
	fStreamData.resize(fStreamCount);
}

Index::~Index()
{
	fStreamData.clear();
}

status_t
Index::GetNextChunkInfo(int stream_index, off_t *start,
							uint32 *size, bool *keyframe) {
	MediaStream *data = &fStreamData[stream_index];
	
	if (data->current_chunk < data->seek_index_next) {
		*keyframe = data->seek_index[data->current_chunk].keyframe;
		// skip 8 bytes (chunk id + chunk size)
		*start = data->seek_index[data->current_chunk].position + 8; 
		*size = data->seek_index[data->current_chunk].size;
		data->current_chunk++;
		return B_OK;
	}
	
	data->current_chunk = 0;

	return B_LAST_BUFFER_ERROR;	// should this be end of chunk?
}

status_t
Index::Seek(int stream_index, uint32 seekTo, int64 *frame,
			bigtime_t *time, bool readOnly) {
	TRACE("Index::Seek: stream %d, seekTo%s%s%s%s, time %Ld, "
		"frame %Ld\n", stream_index,
		(seekTo & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(seekTo & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(seekTo & B_MEDIA_SEEK_CLOSEST_FORWARD) ?
			" B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		(seekTo & B_MEDIA_SEEK_CLOSEST_BACKWARD) ?
			" B_MEDIA_SEEK_CLOSEST_BACKWARD" : "",
		*time, *frame);
	
	const OpenDMLStream *stream = fParser->StreamInfo(stream_index);
	MediaStream *data = &fStreamData[stream_index];

	int64 frame_pos;
	if (seekTo & B_MEDIA_SEEK_TO_FRAME)
		frame_pos = *frame;
	else if (seekTo & B_MEDIA_SEEK_TO_TIME) {
		frame_pos = (*time * stream->frames_per_sec_rate)
			/ (1000000LL * stream->frames_per_sec_scale);
	} else
		return B_BAD_VALUE;

	if (stream->is_audio) {
		// frame_pos is sample no for audio
		// Scan index for the chunk that contains the sample no asked for
		for (uint32 i = 1; i < data->seek_index_next; i++) {
			if (data->seek_index[i].frame_no > frame_pos) {
				// previous chunk contains the frame we wanted
				// we can only seek to chunk boundaries
				frame_pos = data->seek_index[i-1].frame_no;
				if (!readOnly) {
					data->current_chunk = i-1;				// position file to start of chunk
				}
				goto done;
			}
			
			if (i+1 == data->seek_index_next) {
				// Last chunk
				frame_pos = data->seek_index[i].frame_no;
				if (!readOnly) {
					data->current_chunk = i;				// position file to start of chunk
				}
				goto done;
			}
		}
	} else if (stream->is_video) {
		// iterate over all index entries of the stream,
		// there is one entry per frame (TODO: actually one per field,
		// if I interprete the documentation correctly...)
		int64 pos = 0;
		int64 lastKeyframePos = 0;
		int64 lastKeyframeIndex = 0;
		for (uint32 i = 0; i < data->seek_index_next; i++) {
			// remember the last known keyframe index/frame
			if (data->seek_index[i].keyframe) {
				lastKeyframePos = pos;
				lastKeyframeIndex = i;
				TRACE("keyframe at index %ld, frame %Ld (seek: %Ld)\n", i,
					pos, frame_pos);
			}

			if (seekTo & B_MEDIA_SEEK_CLOSEST_BACKWARD) {
				// use the index and frame of the last keyframe
				if (pos == frame_pos) {
					frame_pos = lastKeyframePos;
					if (!readOnly)
						data->current_chunk = lastKeyframeIndex;
					goto done;
				}
			} else if (seekTo & B_MEDIA_SEEK_CLOSEST_FORWARD) {
				// use the index and frame of the last keyframe
				// if this frame is a keyframe and we at or past
				// the seek position
				if (pos >= frame_pos && pos == lastKeyframePos) {
					frame_pos = lastKeyframePos;
					if (!readOnly)
						data->current_chunk = lastKeyframeIndex;
					goto done;
				}
			} else {
				// ignore keyframes
				if (pos == frame_pos) {
					if (!readOnly)
						data->current_chunk = i;
					goto done;
				}
			}
			pos++;
		}
	} else {
		return B_BAD_VALUE;
	}

	ERROR("libOpenDML: seek failed, position not found\n");
	return B_ERROR;

done:
	TRACE("seek done: index: pos %Ld\n", data->current_chunk);
	// recalculate frame and time after seek
	*frame = frame_pos;
	*time = (frame_pos * 1000000LL * stream->frames_per_sec_scale)
		/ stream->frames_per_sec_rate;
	return B_OK;
}

void
Index::AddIndex(int stream_index, off_t position, uint32 size, uint64 frame, bigtime_t pts, bool keyframe) {
	
	IndexEntry seek_index;

	// Should be in a constructor	
	seek_index.frame_no = frame;
	seek_index.position = position;
	seek_index.size = size;
	seek_index.pts = pts;
	seek_index.keyframe = keyframe;
	
	fStreamData[stream_index].seek_index.push_back(seek_index);
	fStreamData[stream_index].seek_index_next++;
}

void 
Index::DumpIndex(int stream_index)
{
	IndexEntry _index;
	for (uint32 i = 0; i < fStreamData[stream_index].seek_index_next; i++) {
		_index = fStreamData[stream_index].seek_index[i];
		printf("index %ld Frame %Ld, pos %Ld, size %ld, pts %Ld\n",i,_index.frame_no, _index.position, _index.size, _index.pts);
	}
}

