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
	StreamEntry streamEntry;

	for (uint32 i=0;i < totalStreams;i++) {
		streamEntry.streamIndex = i;
		streams.push_back(streamEntry);
	}
	
	ParseIndex();
	
	// load the first packet
	if (asf_get_packet(asfFile, packet) < 0) {
		printf("Could not get first packet\n");
		return B_ERROR;
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
				format->FrameScale = stream->extended_properties->avg_time_per_frame;
				format->FrameRate = 10000000L;
				printf("num avg time per frame for video %Ld\n",stream->extended_properties->avg_time_per_frame);
			}
		
			return true;
		}
	}
	
	return false;
}


bigtime_t
ASFFileReader::getStreamDuration(uint32 streamIndex)
{
	if (streamIndex < streams.size()) {
		return streams[streamIndex].getDuration();
	}

	asf_stream_t *stream;

	stream = asf_get_stream(asfFile, streamIndex);
	
	if (stream) {
		if (stream->flags & ASF_STREAM_FLAG_EXTENDED) {
			printf("STREAM %ld end time %Ld, start time %Ld\n",streamIndex, stream->extended_properties->end_time, stream->extended_properties->start_time);
			if (stream->extended_properties->end_time - stream->extended_properties->start_time > 0) {
				return stream->extended_properties->end_time - stream->extended_properties->start_time;
			}
		}
	}

	return asf_get_duration(asfFile) / 10L;
}

uint32
ASFFileReader::getFrameCount(uint32 streamIndex)
{	
	if (streamIndex < streams.size()) {
		return streams[streamIndex].getFrameCount();
	}
	
	return 0;
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

uint32
ASFFileReader::GetFrameForTime(uint32 streamIndex, bigtime_t time)
{
	if (streamIndex < streams.size()) {
		return streams[streamIndex].GetIndex(time).frameNo;
	}
	
	return 0;
}

void
ASFFileReader::ParseIndex() {
	// Try to build some sort of useful index
	// packet->send_time seems to be a better presentation time stamp than pts though	

	if (asf_seek_to_msec(asfFile,0) < 0) {
		printf("Seek to start of stream failed\n");
	}

	asf_payload_t *payload;
	
	while (asf_get_packet(asfFile, packet) > 0) {
		for (int i=0;i<packet->payload_count;i++) {
			payload = (asf_payload_t *)(&packet->payloads[i]);
//			printf("Payload %d Stream %d Keyframe %d send time %Ld pts %Ld id %d size %d\n",i+1,payload->stream_number,payload->key_frame, 1000L * bigtime_t(packet->send_time), 1000L * bigtime_t(payload->pts), payload->media_object_number, payload->datalen);
			if (payload->stream_number < streams.size()) {
				streams[payload->stream_number].AddPayload(payload->media_object_number, payload->key_frame, 1000L * payload->pts, payload->datalen, false);
			}
		}
	}
	
	for (uint32 i=0;i<streams.size();i++) {
		streams[i].AddPayload(0, false, 0, 0, true);
		streams[i].setDuration(1000L * (packet->send_time + packet->duration));
	}
	
	if (asf_seek_to_msec(asfFile,0) < 0) {
		printf("Seek to start of stream failed\n");
	}
}

bool
ASFFileReader::GetNextChunkInfo(uint32 streamIndex, uint32 pFrameNo,
	char **buffer, uint32 *size, bool *keyframe, bigtime_t *pts)
{
	// Ok, Need to join payloads together that have the same payload->media_object_number
	asf_payload_t *payload;
	int64_t seekResult;
	int packetSize;
	
	IndexEntry indexEntry = GetIndex(streamIndex, pFrameNo);
	
	if (indexEntry.noPayloads == 0) {
		// No index entry
		printf("No Index entry for frame %ld\n",pFrameNo);
		return false;
	}
	
//	printf("Stream %ld need pts %Ld, packet start %Ld packet end %Ld\n",streamIndex,indexEntry.pts,1000LL * packet->send_time,1000LL * (packet->send_time + packet->duration));

	if (1000LL * packet->send_time > indexEntry.pts || 1000LL * (packet->send_time + packet->duration) < indexEntry.pts) {
		seekResult = asf_seek_to_msec(asfFile, indexEntry.pts/1000);
		if (seekResult >= 0) {
//			printf("Stream %ld seeked to %Ld got %Ld\n",streamIndex,indexEntry.pts, 1000L * seekResult);
			packetSize = asf_get_packet(asfFile, packet);
			if (packetSize <= 0) {
				printf("Failed to Get Packet after seek result (%d)\n",packetSize);
				return false;
			}
		} else if (seekResult == ASF_ERROR_SEEKABLE) {
			// Stream not seekeable.  Is what we want forward in the stream, if so seek using Get Packet
			if (1000LL * (packet->send_time + packet->duration) < indexEntry.pts) {
				while (1000LL * (packet->send_time + packet->duration) < indexEntry.pts) {
					packetSize = asf_get_packet(asfFile, packet);
					if (packetSize <= 0) {
						printf("Failed to Seek using Get Packet result (%d)\n",packetSize);
						return false;
					}
//					printf("Stream %ld searching forward for pts %Ld, got packet start %Ld packet end %Ld\n",streamIndex,indexEntry.pts,1000LL * packet->send_time,1000LL * (packet->send_time + packet->duration));
				}
			} else {
				// seek to 0 and read forward, going to be a killer on performance
				seekResult = asf_seek_to_msec(asfFile, 0);
				while (1000LL * (packet->send_time + packet->duration) < indexEntry.pts) {
					packetSize = asf_get_packet(asfFile, packet);
					if (packetSize <= 0) {
						printf("Failed to Seek using Get Packet result (%d)\n",packetSize);
						return false;
					}
//					printf("Stream %ld searching forward from 0 for pts %Ld, got packet start %Ld packet end %Ld\n",streamIndex,indexEntry.pts,1000LL * packet->send_time,1000LL * (packet->send_time + packet->duration));
				}
			}
		} else {
			printf("Seek failed\n");
			return false;
		}
	}
	
	// fillin some details
	*size = indexEntry.dataSize;
	*keyframe = indexEntry.keyFrame;
	*pts = indexEntry.pts;
	
	uint32 expectedPayloads = indexEntry.noPayloads;
	uint32 offset = 0;

	for (int i=0;i<packet->payload_count;i++) {
		payload = (asf_payload_t *)(&packet->payloads[i]);
		// find the first payload matching the id we want and then
		// combine the next x payloads where x is the noPayloads in indexEntry
		if (payload->media_object_number == indexEntry.id && payload->stream_number == streamIndex) {
			// copy data to buffer
			memcpy(*buffer + offset, payload->data, payload->datalen);
			offset += payload->datalen;
			expectedPayloads--;
			
			if (expectedPayloads == 0) {
				return true;
			}
		}
	}
	
	// combine packets into a single buffer
	packetSize = asf_get_packet(asfFile, packet);
	while ((packetSize > 0) && (expectedPayloads > 0)) {
		for (int i=0;i<packet->payload_count;i++) {
			payload = (asf_payload_t *)(&packet->payloads[i]);
			// find the first payload matching the id we want and then
			// combine the next x payloads where x is the noPayloads in indexEntry
			if (payload->media_object_number == indexEntry.id && payload->stream_number == streamIndex) {
				// copy data to buffer
				memcpy(*buffer + offset, payload->data, payload->datalen);
				offset += payload->datalen;
				expectedPayloads--;
				if (expectedPayloads == 0) {
					return true;
				}
			}
		}
		packetSize = asf_get_packet(asfFile, packet);
	}
	
	if (packetSize == ASF_ERROR_EOF) {
		printf("Unexpected EOF file truncated?\n");
	} else {
		printf("EOF? %ld,%d\n",expectedPayloads, packetSize);
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
