/*
 * Copyright (c) 2005, David McPaul
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


#include "ASFFileReader.h"

#include <DataIO.h>
#include <SupportKit.h>

#include <iostream>


ASFFileReader::ASFFileReader(BPositionIO *pStream)
{
	theStream = pStream;

	// Find Size of Stream, need to rethink this for non seekable streams
	theStream->Seek(0,SEEK_END);
	StreamSize = theStream->Position();
	theStream->Seek(0,SEEK_SET);
	packet = asf_packet_create();
}


ASFFileReader::~ASFFileReader()
{
	if (packet) {
		asf_packet_destroy(packet);
		packet = NULL;
	}
	
	if (asfFile) {
		asf_close(asfFile);
		asfFile = NULL;
	}
	
	theStream = NULL;
	streams.clear();
}

status_t
ASFFileReader::ParseFile()
{
	asf_iostream_t ioStream;
	ioStream.opaque = theStream;
	ioStream.read = &ASFFileReader::read;
	ioStream.write = &ASFFileReader::write;
	ioStream.seek = &ASFFileReader::seek;
	
	asfFile = asf_open_cb(&ioStream);
	
	int result = asf_init(asfFile);
	
	if (result != 0) {
		printf("error initialising asf asf_init returned %d\n",result);
		return B_ERROR;
	}
	
	asf_metadata_t *metadata = asf_header_get_metadata(asfFile);
	
	if (metadata) {
		printf("Title %s\n",metadata->title);
		printf("Artist %s\n",metadata->artist);
		printf("Copyright %s\n",metadata->copyright);
		printf("Description %s\n",metadata->description);
		printf("Rating %s\n",metadata->rating);
		printf("Additional Entries %d\n",metadata->extended_count);
	
		asf_metadata_destroy(metadata);
	}
	
	uint32 totalStreams = getStreamCount();
	StreamHeader streamHeader;

	for (int i=0;i < totalStreams;i++) {
		streamHeader.streamIndex = i;
		streams.push_back(streamHeader);
	}
	
	return B_OK;
}


bool
ASFFileReader::IsEndOfData(off_t pPosition)
{
	return true;
}


bool
ASFFileReader::IsEndOfFile(off_t position)
{
	return (position >= StreamSize);
}


bool
ASFFileReader::IsEndOfFile()
{
	return theStream->Position() >= StreamSize;
}

uint32
ASFFileReader::getStreamCount()
{
	return asf_get_stream_count(asfFile) + 1;
}

bool
ASFFileReader::getAudioFormat(uint32 streamIndex, ASFAudioFormat *format)
{
	asf_waveformatex_t *audioHeader;
	asf_stream_t *stream;

	if (IsAudio(streamIndex)) {
		stream = asf_get_stream(asfFile, streamIndex);
	
		if (stream) {
			audioHeader = (asf_waveformatex_t *)(stream->properties);
			format->Compression = audioHeader->wFormatTag;
			format->NoChannels = audioHeader->nChannels;
			format->SamplesPerSec = audioHeader->nSamplesPerSec;
			format->AvgBytesPerSec = audioHeader->nAvgBytesPerSec;
			format->BlockAlign = audioHeader->nBlockAlign;
			format->BitsPerSample = audioHeader->wBitsPerSample;
			format->extraDataSize = audioHeader->cbSize;
			format->extraData = audioHeader->data;
		
			if (stream->flags & ASF_STREAM_FLAG_EXTENDED) {
				printf("num payloads for audio %d\n",stream->extended->num_payload_ext);
			}

			return true;
		}
	}
	
	return false;
}

bool
ASFFileReader::getVideoFormat(uint32 streamIndex, ASFVideoFormat *format)
{
	asf_bitmapinfoheader_t *videoHeader;
	asf_stream_t *stream;

	if (IsVideo(streamIndex)) {
		stream = asf_get_stream(asfFile, streamIndex);
	
		if (stream) {
			videoHeader = (asf_bitmapinfoheader_t *)(stream->properties);
			format->Compression = videoHeader->biCompression;
			format->VideoWidth = videoHeader->biWidth;
			format->VideoHeight = videoHeader->biHeight;
			format->Planes = videoHeader->biPlanes;
			format->BitCount = videoHeader->biBitCount;
			format->extraDataSize = videoHeader->biSize - 40;
			format->extraData = videoHeader->data;
			
			if (stream->flags & ASF_STREAM_FLAG_EXTENDED) {
				format->FrameScale = stream->extended->avg_time_per_frame;
				format->FrameRate = 10000000;
				printf("num payloads for video %d\n",stream->extended->num_payload_ext);
			}
		
			return true;
		}
	}
	
	return false;
}


bigtime_t
ASFFileReader::getVideoDuration(uint32 streamIndex)
{
	asf_stream_t *stream;

	stream = asf_get_stream(asfFile, streamIndex);
	
	if (stream) {
		if (stream->flags & ASF_STREAM_FLAG_EXTENDED) {
			printf("VIDEO end time %Ld, start time %Ld\n",stream->extended->end_time, stream->extended->start_time);
			if (stream->extended->end_time - stream->extended->start_time > 0) {
				return stream->extended->end_time - stream->extended->start_time;
			}
		}
	}

	return asf_get_duration(asfFile) / 10;
}


bigtime_t
ASFFileReader::getAudioDuration(uint32 streamIndex)
{
	asf_stream_t *stream;

	stream = asf_get_stream(asfFile, streamIndex);
	
	if (stream) {
		if (stream->flags & ASF_STREAM_FLAG_EXTENDED) {
			printf("AUDIO end time %Ld, start time %Ld\n",stream->extended->end_time, stream->extended->start_time);
			if (stream->extended->end_time - stream->extended->start_time > 0) {
				return stream->extended->end_time - stream->extended->start_time;
			}
		}
	}

	return asf_get_duration(asfFile) / 10;  // convert from 100 nanosecond units to microseconds
}


bigtime_t
ASFFileReader::getMaxDuration()
{
	return asf_get_duration(asfFile) / 10;
}


// Not really frame count, really total data packets
uint32
ASFFileReader::getFrameCount(uint32 streamIndex)
{	
	return asf_get_data_packets(asfFile);
}

uint32
ASFFileReader::getAudioChunkCount(uint32 streamIndex)
{
	return asf_get_data_packets(asfFile);
}

bool
ASFFileReader::IsVideo(uint32 streamIndex)
{
	asf_stream_t *stream;
	
	stream = asf_get_stream(asfFile, streamIndex);
	
	if (stream) {
		return ((stream->type == ASF_STREAM_TYPE_VIDEO) && (stream->flags & ASF_STREAM_FLAG_AVAILABLE));
	}
	
	return false;
}


bool
ASFFileReader::IsAudio(uint32 streamIndex)
{
	asf_stream_t *stream;
	
	stream = asf_get_stream(asfFile, streamIndex);
	
	if (stream) {
		return ((stream->type == ASF_STREAM_TYPE_AUDIO) && (stream->flags & ASF_STREAM_FLAG_AVAILABLE));
	}
	
	return false;
}

void
ASFFileReader::AddIndex(uint32 streamIndex, uint32 frameNo, bool keyFrame, bigtime_t pts, uint8 *data, uint32 size)
{
	streams[streamIndex].AddIndex(frameNo, keyFrame, pts, data, size);
}

IndexEntry
ASFFileReader::GetIndex(uint32 streamIndex, uint32 frameNo) 
{
	return streams[streamIndex].GetIndex(frameNo);
}

bool
ASFFileReader::HasIndex(uint32 streamIndex, uint32 frameNo)
{
	if (streamIndex < streams.size()) {
		return streams[streamIndex].HasIndex(frameNo);
	}
	
	return false;
}

bool
ASFFileReader::GetNextChunkInfo(uint32 streamIndex, uint32 pFrameNo,
	char **buffer, uint32 *size, bool *keyframe, bigtime_t *pts)
{
	// Ok, Need to join payloads together that have the same payload->pts
	// packet->send_time seems to be a better presentation time stamp than pts though	
	
	asf_payload_t *payload;
	
	while (!HasIndex(streamIndex, pFrameNo+1) && asf_get_packet(asfFile, packet) > 0) {
		for (int i=0;i<packet->payload_count;i++) {
			payload = (asf_payload_t *)(&packet->payloads[i]);
			printf("Payload %d ",i+1);
			printf("Stream %d Keyframe %d pts %d frame %d, size %d\n",payload->stream_number,payload->key_frame, payload->pts * 1000, payload->media_object_number, payload->datalen);
			AddIndex(payload->stream_number, payload->media_object_number, payload->key_frame, packet->send_time * 1000, payload->data, payload->datalen);
		}
	}
	
	if (HasIndex(streamIndex, pFrameNo+1)) {
		IndexEntry indexEntry = GetIndex(streamIndex, pFrameNo+1);
		*buffer = (char *)indexEntry.data;
		*size = indexEntry.size;
		*keyframe = indexEntry.keyFrame;
		*pts = indexEntry.pts;
		return true;
	}
	
	return false;
}


/* static */
bool
ASFFileReader::IsSupported(BPositionIO *source)
{
	// Read first 4 bytes and if they match 30 26 b2 75 we have a asf file
	uint8 header[4];
	bool supported = false;
	
	off_t position = source->Position();
	
	if (source->Read(&header[0],4) == 4) {
		supported = header[0] == 0x30 &&
					header[1] == 0x26 &&
					header[2] == 0xb2 &&
					header[3] == 0x75;
	}
	
	source->Seek(position,SEEK_SET);	
	
	return supported;
}

/* static */
int32_t
ASFFileReader::read(void *opaque, void *buffer, int32_t size)
{
	// opaque is the BPositionIO
	return reinterpret_cast<BPositionIO *>(opaque)->Read(buffer, size);
}

/* static */
int32_t
ASFFileReader::write(void *opaque, void *buffer, int32_t size)
{
	return reinterpret_cast<BPositionIO *>(opaque)->Write(buffer, size);
}

/* static */
int64_t
ASFFileReader::seek(void *opaque, int64_t offset)
{
	return reinterpret_cast<BPositionIO *>(opaque)->Seek(offset, SEEK_SET);
}
