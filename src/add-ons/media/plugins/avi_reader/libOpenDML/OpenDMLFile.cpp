/*
 * Copyright (c) 2004-2007, Marcus Overhagen
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
#include <stdio.h>
#include "OpenDMLFile.h"
#include "OpenDMLIndex.h"
#include "StandardIndex.h"
#include "FallbackIndex.h"

//#define TRACE_ODML_FILE
#ifdef TRACE_ODML_FILE
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)

#if 0
#define INDEX_CHUNK_SIZE 32768

struct OpenDMLFile::stream_data
{
	const stream_info *info;

	uint32 	chunk_id;
	
	bool	has_odml_index;
	bool	has_standard_index;

	char *	odml_superindex;
	int		odml_superindex_entry_size;
	int		odml_superindex_entry_count;
	int		odml_superindex_entry_pos;

	// index info (odml_superindex entry, or standard avi index)
	int64	index_base_offset;
	int64	index_entry_start;
	int		index_entry_size;
	int		index_entry_count;
	int		index_entry_pos;

	// index chunk
	char *	index_chunk;
	int		index_chunk_entry_count;
	int		index_chunk_entry_pos;
};
#endif

OpenDMLFile::OpenDMLFile(BPositionIO *source)
 : 	fSource(source)
 ,	fParser(NULL)
 ,	fIndex(NULL)
{
}

OpenDMLFile::~OpenDMLFile()
{
	delete fParser;
	delete fIndex;
}

/* static */ bool
OpenDMLFile::IsSupported(BPositionIO *source)
{
	uint8 h[12];
	if (source->ReadAt(0, h, 12) != 12)
		return false;
	return h[0] == 'R' && h[1] == 'I' && h[2] == 'F' && h[3] == 'F' &&
		   h[8] == 'A' && h[9] == 'V' && h[10] == 'I' && h[11] == ' ';
}

status_t
OpenDMLFile::Init()
{
	fParser = new OpenDMLParser(fSource);

	if (fParser->Init() < B_OK) {
		ERROR("OpenDMLFile::SetTo: parser init failed\n");
		return B_ERROR;
	}

	if (fParser->StreamInfo(0)->odml_index_size != 0) {
		fIndex = new OpenDMLIndex(fSource, fParser);
		if (fIndex->Init() < B_OK) {
			delete fIndex;
			fIndex = NULL;
		}
	}
	
	if (!fIndex && fParser->StandardIndexSize() != 0) {
		fIndex = new StandardIndex(fSource, fParser);
		if (fIndex->Init() < B_OK) {
			delete fIndex;
			fIndex = NULL;
		}
	}
	
	if (!fIndex) {
		fIndex = new FallbackIndex(fSource, fParser);
		if (fIndex->Init() < B_OK) {
			delete fIndex;
			fIndex = NULL;
			TRACE("OpenDMLFile::SetTo: init of fallback index failed!\n");
			return B_ERROR;
		}
	}
	
//	fIndex->DumpIndex(1);
	
	TRACE("OpenDMLFile::SetTo: this is a %s AVI file with %d streams\n", fParser->OdmlExtendedHeader() ? "OpenDML" : "standard", fParser->StreamCount());

	return B_OK;
}

#if 0
void
OpenDMLFile::InitData()
{
	delete [] fStreamData;
	
 	fStreamCount = fParser->StreamCount();
 	fStreamData = new stream_data[fStreamCount];
 	
 	// If this file has a standard avi index, determine
 	// start, size and count of entries, so it can be
 	// assigned to each stream when the stream has no
 	// individual OpenDML index
 	int64 standard_index_start;
	int	standard_index_entry_size;
	int	standard_index_entry_count;
	if (fParser->StandardIndexSize()) {
		standard_index_start = fParser->StandardIndexStart();
		standard_index_entry_size = 16;
		standard_index_entry_count = fParser->StandardIndexSize() / 16;
	} else {
		standard_index_start = -1;
		standard_index_entry_size = 0;
		standard_index_entry_count = 0;
	}
 	
 	for (int stream = 0; stream < fStreamCount; stream++) {
 		TRACE("OpenDMLFile::InitData: stream %d\n", stream);
 	
		fStreamData[stream].info = fParser->StreamInfo(stream);
		fStreamData[stream].has_odml_index = false;
		fStreamData[stream].has_standard_index = false;
		
		if (fStreamData[stream].info->odml_index_size) {
	 		TRACE("OpenDMLFile::InitData: index header, start %Ld, size %lu\n", fStreamData[stream].info->odml_index_start, fStreamData[stream].info->odml_index_size);
			
			odml_index_header h;
			
			// XXX error checking + endian conv.
			
			fSource->ReadAt(fStreamData[stream].info->odml_index_start, &h, sizeof(h));
			
			TRACE("longs_per_entry %u\n", h.longs_per_entry);
			TRACE("index_sub_type %u\n", h.index_sub_type);
			TRACE("index_type %u\n", h.index_type);
			TRACE("entries_used %lu\n", h.entries_used);
			TRACE("chunk_id "FOURCC_FORMAT"\n", FOURCC_PARAM(h.chunk_id));
			
			if (h.index_type == AVI_INDEX_OF_INDEXES) {
				int size = h.entries_used * h.longs_per_entry * 4;
				TRACE("OpenDMLFile::InitData: reading odml_superindex of %d bytes\n", size);
				fStreamData[stream].odml_superindex_entry_size = h.longs_per_entry * 4;
				fStreamData[stream].odml_superindex_entry_count = h.entries_used;
				fStreamData[stream].odml_superindex_entry_pos = 0;
				fStreamData[stream].odml_superindex = new char [size];
				fSource->ReadAt(fStreamData[stream].info->odml_index_start + sizeof(h), fStreamData[stream].odml_superindex, size);
				fStreamData[stream].has_odml_index = true;
			} else if (h.index_type == AVI_INDEX_OF_CHUNKS){
				TRACE("OpenDMLFile::InitData: creating fake odml_superindex\n");
				fStreamData[stream].odml_superindex_entry_size = 16;
				fStreamData[stream].odml_superindex_entry_count = 1;
				fStreamData[stream].odml_superindex_entry_pos = 0;
				fStreamData[stream].odml_superindex = new char [16];
				((odml_superindex_entry *)fStreamData[stream].odml_superindex)->start = fStreamData[stream].info->odml_index_start;
				((odml_superindex_entry *)fStreamData[stream].odml_superindex)->size  = fStreamData[stream].info->odml_index_size;
				((odml_superindex_entry *)fStreamData[stream].odml_superindex)->duration = 0;
				fStreamData[stream].has_odml_index = true;
			} else if (h.index_type == AVI_INDEX_IS_DATA){
				ERROR("OpenDMLFile::InitData: AVI_INDEX_IS_DATA not supported\n");
				fStreamData[stream].odml_superindex = 0;
				fStreamData[stream].odml_superindex_entry_count = 0;
				fStreamData[stream].odml_superindex_entry_pos = 0;
			} else {
				ERROR("OpenDMLFile::InitData: index type not recongnized\n");
				fStreamData[stream].odml_superindex = 0;
				fStreamData[stream].odml_superindex_entry_count = 0;
				fStreamData[stream].odml_superindex_entry_pos = 0;
			}
			for (int i = 0; i < fStreamData[stream].odml_superindex_entry_count; i++) {
#ifdef TRACE_ODML_FILE
				odml_superindex_entry *entry = (odml_superindex_entry *) (fStreamData[stream].odml_superindex + i * fStreamData[stream].odml_superindex_entry_size);
				TRACE("odml_superindex entry %d: start %10Ld, size %8ld, duration %lu\n", i, entry->start, entry->size, entry->duration);
#endif
			}
		} else {
			// this stream has no superindex, as it doesn't have an opendml index :=)
			fStreamData[stream].odml_superindex = 0;
			fStreamData[stream].odml_superindex_entry_count = 0;
			fStreamData[stream].odml_superindex_entry_pos = 0;
		}
		
		fStreamData[stream].index_base_offset = 0;
		fStreamData[stream].index_entry_start = 0;
		fStreamData[stream].index_entry_size = 0;
		fStreamData[stream].index_entry_count = 0;
		fStreamData[stream].index_entry_pos = 0;
		fStreamData[stream].index_chunk = new char [INDEX_CHUNK_SIZE];
		fStreamData[stream].index_chunk_entry_count = 0;
		fStreamData[stream].index_chunk_entry_pos = 0;

		// if this stream doesn't have an OpenDML index,
		// but the file has a standard avi index, we can use it :)
		if (!fStreamData[stream].has_odml_index && standard_index_entry_count != 0) {
			fStreamData[stream].has_standard_index = true;
			fStreamData[stream].index_base_offset = fParser->MovieListStart() - 4; // offset includes the 4 byte 'movi' id
			fStreamData[stream].index_entry_start = standard_index_start;
			fStreamData[stream].index_entry_size = standard_index_entry_size;
			fStreamData[stream].index_entry_count = standard_index_entry_count;
		}
 	}
}

bool
OpenDMLFile::OdmlReadIndexInfo(int stream_index)
{
	stream_data *data = &fStreamData[stream_index];

	if (data->odml_superindex_entry_pos >= data->odml_superindex_entry_count) {
		TRACE("reached end of odml_superindex\n");
		return false;
	}

	odml_superindex_entry *entry = (odml_superindex_entry *) (data->odml_superindex + data->odml_superindex_entry_pos * data->odml_superindex_entry_size);

	TRACE("OpenDMLFile::OdmlReadIndexInfo: stream %d, pos %d, start %Ld, size %lu, duration %u\n",
		stream_index, data->odml_superindex_entry_pos, entry->start, entry->size, entry->duration);

	odml_chunk_index_header chunk_index_header;
	
	if (sizeof(chunk_index_header) != fSource->ReadAt(entry->start + 8, &chunk_index_header, sizeof(chunk_index_header))) {
		ERROR("OpenDMLFile::OdmlReadIndexInfo: read error\n");
		return false;
	}

	TRACE("longs_per_entry %u\n", chunk_index_header.longs_per_entry);
	TRACE("index_sub_type %u\n", chunk_index_header.index_sub_type);
	TRACE("index_type %u\n", chunk_index_header.index_type);
	TRACE("entries_used %lu\n", chunk_index_header.entries_used);
	TRACE("chunk_id "FOURCC_FORMAT"\n", FOURCC_PARAM(chunk_index_header.chunk_id));
	TRACE("base_offset %Ld\n", chunk_index_header.base_offset);

	data->index_base_offset = chunk_index_header.base_offset;
	data->index_entry_start = entry->start + sizeof(chunk_index_header) + 8; // skip 8 bytes (chunk id + chunk size)
	data->index_entry_size = chunk_index_header.longs_per_entry * 4;
	data->index_entry_count = chunk_index_header.entries_used;
	data->index_entry_pos = 0;
	
	data->odml_superindex_entry_pos++;
	return true;
}

bool
OpenDMLFile::OdmlReadIndexChunk(int stream_index)
{
	stream_data *data = &fStreamData[stream_index];

	while (data->index_entry_pos >= data->index_entry_count) {
		if (!OdmlReadIndexInfo(stream_index))
			return false;
	}
	
	data->index_chunk_entry_count = min_c(data->index_entry_count - data->index_entry_pos, INDEX_CHUNK_SIZE / data->index_entry_size);
	data->index_chunk_entry_pos = 0;
	int size = data->index_chunk_entry_count * data->index_entry_size;
	int64 start = data->index_entry_start + data->index_entry_pos * data->index_entry_size;

	TRACE("OpenDMLFile::OdmlReadIndexChunk: stream %d, index_chunk_entry_count %d, size %d, start %Ld\n",
		stream_index, data->index_chunk_entry_count, size, start);
	
	if (size != fSource->ReadAt(start, data->index_chunk, size)) {
		ERROR("OpenDMLFile::OdmlReadIndexChunk read error\n");
		return false;
	}

	data->index_entry_pos += data->index_chunk_entry_count;
	return true;
}

bool
OpenDMLFile::OdmlGetNextChunkInfo(int stream_index, int64 *start, uint32 *size, bool *keyframe)
{
	// this function uses the OpenDML index (an OpenDML AVI contains one index per stream)
	
	stream_data *data = &fStreamData[stream_index];

	while (data->index_chunk_entry_pos >= data->index_chunk_entry_count) {
		if (!OdmlReadIndexChunk(stream_index))
			return false;
	}
	
	odml_index_entry *entry = (odml_index_entry *)(data->index_chunk + data->index_chunk_entry_pos * data->index_entry_size);
	
	*start = data->index_base_offset + entry->start;
	*size = entry->size & 0x7fffffff;
	*keyframe = (entry->size & 0x80000000) ? false : true;
	
	data->index_chunk_entry_pos++;
	
//	TRACE("OpenDMLFile::OdmlGetNextChunkInfo: stream %d: start %15Ld, size %6ld%s\n",
//		stream_index, *start, *size, *keyframe ? ", keyframe" : "");
	return true;
}

bool
OpenDMLFile::AviGetNextChunkInfo(int stream_index, int64 *start, uint32 *size, bool *keyframe)
{
	// this function uses the AVI standard index (a standard AVI contains one index per file, that covers all streams)
	
	stream_data *data = &fStreamData[stream_index];
	
	while (data->index_entry_pos < data->index_entry_count) {
		if (data->index_chunk_entry_pos == data->index_chunk_entry_count) {
			data->index_chunk_entry_count = min_c(data->index_entry_count - data->index_entry_pos, INDEX_CHUNK_SIZE / data->index_entry_size);
			data->index_chunk_entry_pos = 0;
			uint32 start = data->index_entry_start + data->index_entry_pos * data->index_entry_size;
			int size = data->index_chunk_entry_count * data->index_entry_size;
//			TRACE("OpenDMLFile::AviGetNextChunkInfo: stream %d, index_chunk_entry_count %d, size %d, start %lu\n",
//				stream_index, data->index_chunk_entry_count, size, start);
			if (size != fSource->ReadAt(start, data->index_chunk, size)) {
				ERROR("OpenDMLFile::AviGetNextChunkInfo: read error\n");
				return false;
			}
		}

		avi_standard_index_entry *entry = (avi_standard_index_entry *)(data->index_chunk + data->index_chunk_entry_pos * data->index_entry_size);
		
//		TRACE("OpenDMLFile::AviGetNextChunkInfo: stream %d, chunk_id "FOURCC_FORMAT" (0x%08x), flags 0x%x, offset %lu, length %lu\n",
//			 stream_index, FOURCC_PARAM(entry->chunk_id), entry->chunk_id, entry->flags, entry->chunk_offset, entry->chunk_length);

		data->index_chunk_entry_pos++;		
		data->index_entry_pos++;
			 
		int real_stream_index = ((entry->chunk_id & 0xff) - '0') * 10 + ((entry->chunk_id >> 8) & 0xff) - '0';

//		TRACE("OpenDMLFile::AviGetNextChunkInfo: real_stream_index %d %d %d\n", real_stream_index, ((entry->chunk_id & 0xff) - '0'), ((entry->chunk_id >> 8) & 0xff) - '0');
			 
		if (real_stream_index == stream_index) {		

//			TRACE("OpenDMLFile::AviGetNextChunkInfo: stream %d, chunk_id "FOURCC_FORMAT" (0x%08x), flags 0x%08x, offset %lu, length %lu\n",
//				 stream_index, FOURCC_PARAM(entry->chunk_id), entry->chunk_id, entry->flags, entry->chunk_offset, entry->chunk_length);

			*keyframe = entry->flags & AVIIF_KEYFRAME;
			*start = data->index_base_offset + entry->chunk_offset + 8;  // skip 8 bytes (chunk id + chunk size)
			*size = entry->chunk_length;
			return true;
		}
		
	}
	TRACE("OpenDMLFile::AviGetNextChunkInfo: index end reached\n");
	return false;
}
#endif


status_t
OpenDMLFile::GetNextChunkInfo(int stream_index, off_t *start, uint32 *size,
	bool *keyframe)
{
	return fIndex->GetNextChunkInfo(stream_index, start, size, keyframe);
}

status_t
OpenDMLFile::Seek(int stream_index, uint32 seekTo, int64 *frame,
	bigtime_t *time, bool readOnly)
{
	return fIndex->Seek(stream_index, seekTo, frame, time, readOnly);
}

int
OpenDMLFile::StreamCount()
{
	return fParser->StreamCount();
}

/*
bigtime_t
OpenDMLFile::Duration()
{
	if (!fParser->AviMainHeader())
		return 0;
	if (fParser->OdmlExtendedHeader())
		return fParser->OdmlExtendedHeader()->total_frames * (bigtime_t)fParser->AviMainHeader()->micro_sec_per_frame;
	return fParser->AviMainHeader()->total_frames * (bigtime_t)fParser->AviMainHeader()->micro_sec_per_frame;
}

uint32
OpenDMLFile::FrameCount()
{
	if (fParser->OdmlExtendedHeader())
		return fParser->OdmlExtendedHeader()->total_frames;
	if (fParser->AviMainHeader())
		return fParser->AviMainHeader()->total_frames;
	return 0;
}
*/

bool
OpenDMLFile::IsVideo(int stream_index)
{
	return fParser->StreamInfo(stream_index)->is_video;
}

bool
OpenDMLFile::IsAudio(int stream_index)
{
	return fParser->StreamInfo(stream_index)->is_audio;
}

const avi_main_header *
OpenDMLFile::AviMainHeader()
{
	return fParser->AviMainHeader();
}

const wave_format_ex *
OpenDMLFile::AudioFormat(int stream_index, size_t *size /* = 0*/)
{
	if (!fParser->StreamInfo(stream_index)->is_audio)
		return 0;
	if (!fParser->StreamInfo(stream_index)->audio_format)
		return 0;
	if (size)
		*size = fParser->StreamInfo(stream_index)->audio_format_size;
	return fParser->StreamInfo(stream_index)->audio_format;
}

const bitmap_info_header *
OpenDMLFile::VideoFormat(int stream_index)
{
	return (fParser->StreamInfo(stream_index)->is_video && fParser->StreamInfo(stream_index)->video_format_valid) ?
		&fParser->StreamInfo(stream_index)->video_format : 0;
}

const avi_stream_header *
OpenDMLFile::StreamFormat(int stream_index)
{
	return (fParser->StreamInfo(stream_index)->stream_header_valid) ?
		&fParser->StreamInfo(stream_index)->stream_header : 0;
}

const OpenDMLStream *
OpenDMLFile::StreamInfo(int index)
{
	return fParser->StreamInfo(index);
}
