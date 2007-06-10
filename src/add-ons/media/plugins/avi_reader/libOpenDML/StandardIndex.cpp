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


struct StandardIndex::stream_data
{
	uint32 chunk_id;
	uint32 pos;
};

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
	delete [] fStreamData;
}


status_t
StandardIndex::Init()
{
	uint32 indexBytes = fParser->StandardIndexSize();
	fIndexSize = indexBytes / sizeof(avi_standard_index_entry);
	indexBytes = fIndexSize * sizeof(avi_standard_index_entry);

 { BStopWatch w("StandardIndex::Init: malloc");

	if (indexBytes > 0x1900000) { // 25 MB
		printf("StandardIndex::Init index is way too big\n");
		return B_NO_MEMORY;
	}

	if (fIndexSize == 0) {
		printf("StandardIndex::Init index is empty\n");
		return B_NO_MEMORY;
	}

	fIndex = new (std::nothrow) avi_standard_index_entry[fIndexSize];
	if (fIndex == NULL) {
		printf("StandardIndex::Init out of memory\n");
		return B_NO_MEMORY;
	}
}
{ BStopWatch w("StandardIndex::Init: file read");

	if (indexBytes != fSource->ReadAt(fParser->StandardIndexStart(), fIndex, indexBytes)) {
		printf("StandardIndex::Init file reading failed\n");
		delete [] fIndex;
		fIndex = NULL;
		return B_IO_ERROR;
	}
}

	fStreamData = new stream_data[fStreamCount];
	for (int i = 0; i < fStreamCount; i++) {
		fStreamData[i].chunk_id = i / 10 + '0' + (i % 10 + '0') * 256;
		fStreamData[i].pos = 0;
	}
	
	//DumpIndex();

	return B_OK;
}


void 
StandardIndex::DumpIndex()
{
	uint32 chunk = fIndex->chunk_id;
	int count = 0;
	int pos = 0;

	printf("StandardIndex::DumpIndex %u entries\n", fIndexSize);
	for (int i = 0; i < fIndexSize; i++) {
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
StandardIndex::GetNextChunkInfo(int stream_index, int64 *start, uint32 *size, bool *keyframe)
{
	stream_data *data = &fStreamData[stream_index];
	while (data->pos < fIndexSize) {
		if ((fIndex[data->pos].chunk_id & 0xffff) == data->chunk_id) {
			*keyframe = fIndex[data->pos].flags & AVIIF_KEYFRAME;
			*start = fDataOffset + fIndex[data->pos].chunk_offset + 8;  // skip 8 bytes (chunk id + chunk size)
			*size = fIndex[data->pos].chunk_length;
			data->pos++;
			return B_OK;
		}
		data->pos++;
	}

	return B_ERROR;
}


status_t
StandardIndex::Seek(int stream_index, uint32 seekTo, int64 *frame, bigtime_t *time)
{
/*
	printf("StandardIndex::Seek: seekTo%s%s%s%s, time %Ld, frame %Ld\n",
		(seekTo & B_MEDIA_SEEK_TO_TIME) ? " B_MEDIA_SEEK_TO_TIME" : "",
		(seekTo & B_MEDIA_SEEK_TO_FRAME) ? " B_MEDIA_SEEK_TO_FRAME" : "",
		(seekTo & B_MEDIA_SEEK_CLOSEST_FORWARD) ? " B_MEDIA_SEEK_CLOSEST_FORWARD" : "",
		(seekTo & B_MEDIA_SEEK_CLOSEST_BACKWARD) ? " B_MEDIA_SEEK_CLOSEST_BACKWARD" : "");
*/
	return B_ERROR;
}

