/*
 * Copyright (c) 2003-2004, Marcus Overhagen
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
#include <string.h>
#include <malloc.h>
#include <DataIO.h>
#include <ByteOrder.h>
#include <InterfaceDefs.h>
#include "RawFormats.h"
#include "aiff_reader.h"

//#define TRACE_AIFF_READER
#ifdef TRACE_AIFF_READER
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define BUFFER_SIZE	16384

#define FOURCC(a,b,c,d)	((((uint32)(a)) << 24) | (((uint32)(b)) << 16) | (((uint32)(c)) << 8) | ((uint32)(d)))
#define UINT16(a) 		((uint16)B_BENDIAN_TO_HOST_INT16((a)))
#define UINT32(a) 		((uint32)B_BENDIAN_TO_HOST_INT32((a)))

aiffReader::aiffReader()
{
	TRACE("aiffReader::aiffReader\n");
	fBuffer = 0;
}


aiffReader::~aiffReader()
{
	if (fBuffer)
		free(fBuffer);
}

      
const char *
aiffReader::Copyright()
{
	return "AIFF & AIFF-C reader, " B_UTF8_COPYRIGHT " by Marcus Overhagen";
}

	
status_t
aiffReader::Sniff(int32 *streamCount)
{
	TRACE("aiffReader::Sniff\n");

	fSource = dynamic_cast<BPositionIO *>(Reader::Source());
	if (!fSource) {
		TRACE("aiffReader::Sniff: not a BPositionIO\n");
		return B_ERROR;
	}

	int64 filesize = Source()->Seek(0, SEEK_END);
	if (filesize < 42) {
		TRACE("aiffReader::Sniff: File too small\n");
		return B_ERROR;
	}
	
	aiff_chunk aiff;

	if (sizeof(aiff) != Source()->ReadAt(0, &aiff, sizeof(aiff))) {
		TRACE("aiffReader::Sniff: aiff_chunk reading failed\n");
		return B_ERROR;
	}

	if (UINT32(aiff.chunk_id) != FOURCC('F','O','R','M')
		|| (UINT32(aiff.aiff_id) != FOURCC('A','I','F','F')
			&& UINT32(aiff.aiff_id) != FOURCC('A','I','F','C'))) {
		TRACE("aiffReader::Sniff: aiff not recognized\n");
		return B_ERROR;
	}
	
	comm_chunk comm;
	ssnd_chunk ssnd;
	int64 pos = sizeof(aiff);

	fDataStart = 0;
	fDataSize = 0;
	
	// read all chunks and search for "COMM" and "SSND", everything else is ignored
	bool foundCOMM = false;
	bool foundSSND = false;
	while (pos + sizeof(chunk_struct) <= filesize) {
		chunk_struct chunk;
		if (sizeof(chunk) != Source()->ReadAt(pos, &chunk, sizeof(chunk))) {
			TRACE("aiffReader::Sniff: chunk header reading failed\n");
			return B_ERROR;
		}
		pos += sizeof(chunk);
		if (UINT32(chunk.chunk_size) == 0) {
			TRACE("aiffReader::Sniff: Error: chunk of size 0 found\n");
			return B_ERROR;
		}
		switch (UINT32(chunk.chunk_id)) {
			case FOURCC('C','O','M','M'):
			{
				if (foundCOMM)
					break;
				uint32 size = UINT32(chunk.chunk_size);
				if (size >= 18) {
					size = min_c(size, sizeof(comm));
					if (size != (uint32)Source()->ReadAt(pos, &comm, size)) {
						TRACE("aiffReader::Sniff: COMM chunk reading failed\n");
						break;
					}
					if (size < 22)
						comm.compression_id = B_HOST_TO_BENDIAN_INT32(FOURCC('N','O','N','E'));
					foundCOMM = true;
				} else {
					TRACE("aiffReader::Sniff: COMM chunk too small\n");
				}
				break;
			}
			case FOURCC('S','S','N','D'):
			{
				if (foundSSND)
					break;
				uint32 size = UINT32(chunk.chunk_size);
				if (size >= sizeof(ssnd)) {
					if (sizeof(ssnd) != Source()->ReadAt(pos, &ssnd, sizeof(ssnd))) {
						TRACE("aiffReader::Sniff: SSND chunk reading failed\n");
						break;
					}
					fDataStart = pos + sizeof(ssnd) + UINT32(ssnd.offset);
					fDataSize = UINT32(chunk.chunk_size) - sizeof(ssnd);
					if (pos + fDataSize + sizeof(ssnd) > filesize)
						fDataSize = filesize - pos - sizeof(ssnd);
					foundSSND = true;
				} else {
					TRACE("aiffReader::Sniff: SSND chunk too small\n");
				}
				break;
			}
			default:
				TRACE("aiffReader::Sniff: ignoring chunk 0x%08lx of %lu bytes\n", UINT32(chunk.chunk_id), UINT32(chunk.chunk_size));
				break;
		}
		pos += UINT32(chunk.chunk_size);
		pos += (pos & 1);
	}

	if (!foundCOMM) {
		TRACE("aiffReader::Sniff: couldn't find format chunk\n");
		return B_ERROR;
	}
	if (!foundSSND) {
		TRACE("aiffReader::Sniff: couldn't find data chunk\n");
		return B_ERROR;
	}

	TRACE("aiffReader::Sniff: we found something that looks like:\n");
	
	TRACE("  channel_count    %d\n", UINT16(comm.channel_count));
	TRACE("  frame_count      %ld\n", UINT32(comm.frame_count));
	TRACE("  bits_per_sample  %d\n", UINT16(comm.bits_per_sample));
	TRACE("  sample_rate      %ld\n", DecodeFrameRate(comm.sample_rate));
	TRACE("  compression_id   %s\n", string_for_compression(UINT32(comm.compression_id)));
	TRACE("  offset           %ld\n", UINT32(ssnd.offset));
	TRACE("  block_size       %ld\n", UINT32(ssnd.block_size));

	fChannelCount = UINT16(comm.channel_count);
	fFrameCount = UINT32(comm.frame_count);
	fFrameRate = DecodeFrameRate(comm.sample_rate);
	fDuration = (1000000LL * fFrameCount) / fFrameRate;
	fValidBitsPerSample = UINT16(comm.bits_per_sample);
	fBytesPerSample = (fValidBitsPerSample + 7) / 8;
	fBytesPerFrame = fBytesPerSample * fChannelCount;
	fFormatCode = UINT32(comm.compression_id);

	if (fChannelCount < 1)
		fChannelCount = 1;
	if (fFrameRate < 1)
		fFrameRate = 44100;

	if (fBytesPerSample == 0) {
		TRACE("aiffReader::Sniff: sample format not recognized\n");
		return B_ERROR;
	}
	
	switch (fFormatCode) {
		case FOURCC('N','O','N','E'):
		case FOURCC('F','L','3','2'):
		case FOURCC('f','l','3','2'):
		case FOURCC('f','l','6','4'):
			fRaw = true;
			break;
		default:
			fRaw = false;
			break;
	}

	fPosition = 0;
	
	fBufferSize = (BUFFER_SIZE / fBytesPerFrame) * fBytesPerFrame;
	fBuffer = malloc(fBufferSize);

	TRACE("  fDataStart     %Ld\n", fDataStart);
	TRACE("  fDataSize      %Ld\n", fDataSize);
	TRACE("  fFrameCount    %Ld\n", fFrameCount);
	TRACE("  fDuration      %Ld\n", fDuration);
	TRACE("  fChannelCount  %d\n", fChannelCount);
	TRACE("  fFrameRate     %ld\n", fFrameRate);
	TRACE("  fValidBitsPerSample %d\n", fValidBitsPerSample);
	TRACE("  fBytesPerSample     %d\n", fBytesPerSample);
	TRACE("  fBytesPerFrame %d\n", fBytesPerFrame);
	TRACE("  fFormatCode    %ld\n", fFormatCode);
	TRACE("  fRaw           %d\n", fRaw);
	
	BMediaFormats formats;
	if (fRaw) {
		// a raw PCM format
		media_format_description description;
		description.family = B_BEOS_FORMAT_FAMILY;
		description.u.beos.format = B_BEOS_FORMAT_RAW_AUDIO;
		formats.GetFormatFor(description, &fFormat);
		fFormat.u.raw_audio.frame_rate = fFrameRate;
		fFormat.u.raw_audio.channel_count = fChannelCount;
		switch (fFormatCode) {
			case FOURCC('N','O','N','E'):
				switch (fBytesPerSample) {
					case 1:
						fFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_CHAR;
						break;
					case 2:
						fFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
						break;
					case 3:
						fFormat.u.raw_audio.format = B_AUDIO_FORMAT_INT24;
						break;
					case 4:
						fFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_INT;
						break;
					default:
						TRACE("aiffReader::Sniff: unknown bytes per sample for raw format\n");
						return B_ERROR;
				}
				break;
			case FOURCC('F','L','3','2'):
				fFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
				break;
			case FOURCC('f','l','3','2'):
				fFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
				break;
			case FOURCC('f','l','6','4'):
				fFormat.u.raw_audio.format = B_AUDIO_FORMAT_FLOAT64;
				break;
			default:
				TRACE("aiffReader::Sniff: unknown raw format\n");
				return B_ERROR;
		}
		fFormat.u.raw_audio.byte_order = B_MEDIA_BIG_ENDIAN;
		fFormat.u.raw_audio.buffer_size = fBufferSize;
	} else {
		// some encoded format
		media_format_description description;
		description.family = B_AIFF_FORMAT_FAMILY;
		description.u.aiff.codec = fFormatCode;
		formats.GetFormatFor(description, &fFormat);
		fFormat.u.encoded_audio.output.frame_rate = fFrameRate;
		fFormat.u.encoded_audio.output.channel_count = fChannelCount;
	}

	*streamCount = 1;
	return B_OK;
}

void
aiffReader::GetFileFormatInfo(media_file_format *mff)
{
	mff->capabilities =   media_file_format::B_READABLE
						| media_file_format::B_KNOWS_ENCODED_AUDIO
						| media_file_format::B_IMPERFECTLY_SEEKABLE;
	mff->family = B_MISC_FORMAT_FAMILY;
	mff->version = 100;
	strcpy(mff->mime_type, "audio/x-aiff");
	strcpy(mff->file_extension, "aiff");
	strcpy(mff->short_name,  "AIFF / AIFF-C");
	strcpy(mff->pretty_name, "Audio Interchange File Format");
}

status_t
aiffReader::AllocateCookie(int32 streamNumber, void **cookie)
{
	return B_OK;
}

status_t
aiffReader::FreeCookie(void *cookie)
{
	return B_OK;
}


status_t
aiffReader::GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
						 media_format *format, const void **infoBuffer, size_t *infoSize)
{
	*frameCount = fFrameCount;
	*duration = fDuration;
	*format = fFormat;
	*infoBuffer = 0;
	*infoSize = 0;
	return B_OK;
}


status_t
aiffReader::Seek(void *cookie,
				uint32 seekTo,
				int64 *frame, bigtime_t *time)
{
	int64 pos;

	if (seekTo & B_MEDIA_SEEK_TO_FRAME) {
		if (fRaw)
			pos = *frame * fBytesPerFrame;
		else
			pos = (*frame * fDataSize) / fFrameCount;
		pos = (pos / fBytesPerFrame) * fBytesPerFrame; // round down to a block start
		TRACE("aiffReader::Seek to frame %Ld, pos %Ld\n", *frame, pos);
	} else if (seekTo & B_MEDIA_SEEK_TO_TIME) {
		if (fRaw)
			pos = (*time * fFrameRate * fBytesPerFrame) / 1000000LL;
		else
			pos = (*time * fDataSize) / fDuration;
		pos = (pos / fBytesPerFrame) * fBytesPerFrame; // round down to a block start
		TRACE("aiffReader::Seek to time %Ld, pos %Ld\n", *time, pos);
	} else {
		return B_ERROR;
	}

	if (fRaw)
		*frame = pos / fBytesPerFrame;
	else
		*frame = (pos * fFrameCount) / fDataSize;
	*time = (*frame * 1000000LL) / fFrameRate;

	TRACE("aiffReader::Seek newtime %Ld\n", *time);
	TRACE("aiffReader::Seek newframe %Ld\n", *frame);
	
	if (pos < 0 || pos > fDataSize) {
		TRACE("aiffReader::Seek invalid position %Ld\n", pos);
		return B_ERROR;
	}
	
	fPosition = pos;
	return B_OK;
}


status_t
aiffReader::GetNextChunk(void *cookie,
						const void **chunkBuffer, size_t *chunkSize,
						media_header *mediaHeader)
{
	// XXX it might be much better to not return any start_time information for encoded formats here,
	// XXX and instead use the last time returned from seek and count forward after decoding.
	mediaHeader->start_time = ((fPosition / fBytesPerFrame) * 1000000LL) / fFrameRate;
	mediaHeader->file_pos = fDataStart + fPosition;

	int64 maxreadsize = fDataSize - fPosition;
	int32 readsize = fBufferSize;
	if (maxreadsize < readsize)
		readsize = maxreadsize;
	if (readsize == 0)
		return B_LAST_BUFFER_ERROR;
		
	if (readsize != Source()->ReadAt(fDataStart + fPosition, fBuffer, readsize)) {
		TRACE("aiffReader::GetNextChunk: unexpected read error\n");
		return B_ERROR;
	}
	
	// XXX if the stream has more than two channels, we need to reorder channel data here
	
	fPosition += readsize;
	*chunkBuffer = fBuffer;
	*chunkSize = readsize;
	return B_OK;
}

uint32
aiffReader::DecodeFrameRate(const void *_80bit_float)
{
	// algorithm from http://www.borg.com/~jglatt/tech/aiff.htm
	uint32 mantissa;
	uint32 last;
	uint32 exp;

	mantissa = (uint32)B_BENDIAN_TO_HOST_INT32(*(const uint32 *)((const char *)_80bit_float + 2));
	exp = 30 - *(const uint8 *)((const char *)_80bit_float + 1);
	if (exp > 32)
		return 0;
	last = 0;
	while (exp--) {
		last = mantissa;
		mantissa >>= 1;
	}
	if (last & 1)
		mantissa++; 
   return mantissa; 
}


Reader *
aiffReaderPlugin::NewReader()
{
	return new aiffReader;
}


MediaPlugin *instantiate_plugin()
{
	return new aiffReaderPlugin;
}
