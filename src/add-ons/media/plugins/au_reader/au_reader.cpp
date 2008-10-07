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
#include "au_reader.h"

//#define TRACE_AU_READER
#ifdef TRACE_AU_READER
  #define TRACE printf
#else
  #define TRACE(a...)
#endif

#define BUFFER_SIZE	16384

#define UINT32(a) 		((uint32)B_BENDIAN_TO_HOST_INT32((a)))

auReader::auReader()
{
	TRACE("auReader::auReader\n");
	fBuffer = 0;
}


auReader::~auReader()
{
	if (fBuffer)
		free(fBuffer);
}

      
const char *
auReader::Copyright()
{
	return ".au & .snd reader, " B_UTF8_COPYRIGHT " by Marcus Overhagen";
}

	
status_t
auReader::Sniff(int32 *streamCount)
{
	TRACE("auReader::Sniff\n");

	fSource = dynamic_cast<BPositionIO *>(Reader::Source());
	if (!fSource) {
		TRACE("auReader::Sniff: not a BPositionIO\n");
		return B_ERROR;
	}

	int64 filesize = Source()->Seek(0, SEEK_END);
	if (filesize < sizeof(struct snd_header)) {
		TRACE("auReader::Sniff: File too small\n");
		return B_ERROR;
	}
	
	struct snd_header header;

	if (sizeof(header) != Source()->ReadAt(0, &header, sizeof(header))) {
		TRACE("auReader::Sniff: header reading failed\n");
		return B_ERROR;
	}

	if (UINT32(header.magic) != SND_MAGIC) {
		TRACE("auReader::Sniff: header not recognized\n");
		return B_ERROR;
	}
	
	TRACE("auReader::Sniff: we found something that looks like:\n");
	
	TRACE("  data_start        %ld\n", UINT32(header.data_start));
	TRACE("  data_size         %ld\n", UINT32(header.data_size));
	TRACE("  data_format       %ld\n", UINT32(header.data_format));
	TRACE("  sampling_rate     %ld\n", UINT32(header.sampling_rate));
	TRACE("  channel_count     %ld\n", UINT32(header.channel_count));

	fDataStart = UINT32(header.data_start);
	fDataSize = UINT32(header.data_size);
	fChannelCount = UINT32(header.channel_count);
	fFrameRate = UINT32(header.sampling_rate);
	fFormatCode = UINT32(header.data_format);
	
	if (fDataStart > filesize) {
		TRACE("auReader::Sniff: data start too large\n");
		return B_ERROR;
	}
	if (fDataStart + fDataSize > filesize)
		fDataSize = filesize - fDataStart;
	if (fDataSize < 1) {
		TRACE("auReader::Sniff: data size too small\n");
		return B_ERROR;
	}
	if (fChannelCount < 1)
		fChannelCount = 1;
	if (fFrameRate < 1)
		fFrameRate = 44100;

	switch (fFormatCode) {
		case SND_FORMAT_UNSPECIFIED: TRACE("SND_FORMAT_UNSPECIFIED\n"); break;
		case SND_FORMAT_MULAW_8: TRACE("SND_FORMAT_MULAW_8\n"); break;
		case SND_FORMAT_LINEAR_8: TRACE("SND_FORMAT_LINEAR_8\n"); break;
		case SND_FORMAT_LINEAR_16: TRACE("SND_FORMAT_LINEAR_16\n"); break;
		case SND_FORMAT_LINEAR_24: TRACE("SND_FORMAT_LINEAR_24\n"); break;
		case SND_FORMAT_LINEAR_32: TRACE("SND_FORMAT_LINEAR_32\n"); break;
		case SND_FORMAT_FLOAT: TRACE("SND_FORMAT_FLOAT\n"); break;
		case SND_FORMAT_DOUBLE: TRACE("SND_FORMAT_DOUBLE\n"); break;
		case SND_FORMAT_INDIRECT: TRACE("SND_FORMAT_INDIRECT\n"); break;
		case SND_FORMAT_NESTED: TRACE("SND_FORMAT_NESTED\n"); break;
		case SND_FORMAT_DSP_CORE: TRACE("SND_FORMAT_DSP_CORE\n"); break;
		case SND_FORMAT_DSP_DATA_8: TRACE("SND_FORMAT_DSP_DATA_8\n"); break;
		case SND_FORMAT_DSP_DATA_16: TRACE("SND_FORMAT_DSP_DATA_16\n"); break;
		case SND_FORMAT_DSP_DATA_24: TRACE("SND_FORMAT_DSP_DATA_24\n"); break;
		case SND_FORMAT_DSP_DATA_32: TRACE("SND_FORMAT_DSP_DATA_32\n"); break;
		case SND_FORMAT_DISPLAY: TRACE("SND_FORMAT_DISPLAY\n"); break;
		case SND_FORMAT_MULAW_SQUELCH: TRACE("SND_FORMAT_MULAW_SQUELCH\n"); break;
		case SND_FORMAT_EMPHASIZED: TRACE("SND_FORMAT_EMPHASIZED\n"); break;
		case SND_FORMAT_COMPRESSED: TRACE("SND_FORMAT_COMPRESSED\n"); break;
		case SND_FORMAT_COMPRESSED_EMPHASIZED: TRACE("SND_FORMAT_COMPRESSED_EMPHASIZED\n"); break;
		case SND_FORMAT_DSP_COMMANDS: TRACE("SND_FORMAT_DSP_COMMANDS\n"); break;
		case SND_FORMAT_DSP_COMMANDS_SAMPLES: TRACE("SND_FORMAT_DSP_COMMANDS_SAMPLES\n"); break;
		case SND_FORMAT_ADPCM_G721: TRACE("SND_FORMAT_ADPCM_G721\n"); break;
		case SND_FORMAT_ADPCM_G722: TRACE("SND_FORMAT_ADPCM_G722\n"); break;
		case SND_FORMAT_ADPCM_G723_3: TRACE("SND_FORMAT_ADPCM_G723_3\n"); break;
		case SND_FORMAT_ADPCM_G723_5: TRACE("SND_FORMAT_ADPCM_G723_5\n"); break;
		case SND_FORMAT_ALAW_8: TRACE("SND_FORMAT_ALAW_8\n"); break;
	}

	switch (fFormatCode) {
		case SND_FORMAT_MULAW_8:
			fBitsPerSample = 8; fRaw = false; break;
		case SND_FORMAT_LINEAR_8:
			fBitsPerSample = 8; fRaw = true; break;
		case SND_FORMAT_LINEAR_16:
			fBitsPerSample = 16; fRaw = true; break;
		case SND_FORMAT_LINEAR_24:
			fBitsPerSample = 24; fRaw = true; break;
		case SND_FORMAT_LINEAR_32:
			fBitsPerSample = 32; fRaw = true; break;
		case SND_FORMAT_FLOAT:
			fBitsPerSample = 32; fRaw = true; break;
		case SND_FORMAT_DOUBLE:
			fBitsPerSample = 64; fRaw = true; break;
		case SND_FORMAT_ADPCM_G721:
			fBitsPerSample = 4; fRaw = false; break;
		case SND_FORMAT_ADPCM_G722:
			fBitsPerSample = 8; fRaw = false; break;
		case SND_FORMAT_ADPCM_G723_3:
			fBitsPerSample = 3; fRaw = false; break;
		case SND_FORMAT_ADPCM_G723_5:
			fBitsPerSample = 5; fRaw = false; break;
		case SND_FORMAT_ALAW_8:
			fBitsPerSample = 8; fRaw = false; break;
		default:
			fBitsPerSample = 0; break;
	}
	if (fBitsPerSample == 0) {
		TRACE("auReader::Sniff: sample format not recognized\n");
		return B_ERROR;
	}
	
	fFrameCount = (8 * fDataSize) / (fChannelCount * fBitsPerSample);
	fDuration = (1000000LL * fFrameCount) / fFrameRate;
	fBitsPerFrame = fChannelCount * fBitsPerSample;
	fBlockAlign = fBitsPerFrame;
	while (fBlockAlign % 8 && fBlockAlign < 1000)
		fBlockAlign += fBlockAlign;
	if (fBlockAlign % 8) {
		TRACE("auReader::Sniff: can't find block alignment, fChannelCount %d, fBitsPerSample %d\n", fChannelCount, fBitsPerSample);
		return B_ERROR;
	}
	fBlockAlign /= 8;

	fPosition = 0;
	
	fBufferSize = (BUFFER_SIZE / fBlockAlign) * fBlockAlign;
	fBuffer = malloc(fBufferSize);

	TRACE("  fDataStart     %Ld\n", fDataStart);
	TRACE("  fDataSize      %Ld\n", fDataSize);
	TRACE("  fFrameCount    %Ld\n", fFrameCount);
	TRACE("  fDuration      %Ld\n", fDuration);
	TRACE("  fChannelCount  %d\n", fChannelCount);
	TRACE("  fFrameRate     %ld\n", fFrameRate);
	TRACE("  fBitsPerSample %d\n", fBitsPerSample);
	TRACE("  fBlockAlign    %d\n", fBlockAlign);
	TRACE("  fFormatCode    %ld\n", fFormatCode);
	TRACE("  fRaw           %d\n", fRaw);
	
	BMediaFormats formats;
	if (fRaw) {
		// a raw PCM format
		media_format_description description;
		description.family = B_BEOS_FORMAT_FAMILY;
		description.u.beos.format = B_BEOS_FORMAT_RAW_AUDIO;
		formats.GetFormatFor(description, &fFormat);
		fFormat.u.raw_audio.frame_rate = (fFrameRate == 8012) ? SND_RATE_8012 : fFrameRate;
		fFormat.u.raw_audio.channel_count = fChannelCount;
		switch (fFormatCode) {
			case SND_FORMAT_LINEAR_8:
				fFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_UCHAR;
				break;
			case SND_FORMAT_LINEAR_16:
				fFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
				break;
			case SND_FORMAT_LINEAR_24:
				fFormat.u.raw_audio.format = B_AUDIO_FORMAT_INT24;
				break;
			case SND_FORMAT_LINEAR_32:
				fFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_INT;
				break;
			case SND_FORMAT_FLOAT:
				fFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
				break;
			case SND_FORMAT_DOUBLE:
				fFormat.u.raw_audio.format = B_AUDIO_FORMAT_FLOAT64;
				break;
			default:
				TRACE("auReader::Sniff: unhandled raw format\n");
				return B_ERROR;
		}
		fFormat.u.raw_audio.byte_order = B_MEDIA_BIG_ENDIAN;
		fFormat.u.raw_audio.buffer_size = fBufferSize;
	} else {
		// some encoded format
		media_format_description description;
		description.family = B_MISC_FORMAT_FAMILY;
		description.u.misc.file_format = 'au';
		description.u.misc.codec = fFormatCode;
		formats.GetFormatFor(description, &fFormat);
		fFormat.u.encoded_audio.output.frame_rate = fFrameRate;
		fFormat.u.encoded_audio.output.channel_count = fChannelCount;
	}
	
	*streamCount = 1;
	return B_OK;
}


void
auReader::GetFileFormatInfo(media_file_format *mff)
{
	mff->capabilities =   media_file_format::B_READABLE
						| media_file_format::B_KNOWS_RAW_AUDIO
						| media_file_format::B_KNOWS_ENCODED_AUDIO
						| media_file_format::B_IMPERFECTLY_SEEKABLE;
	mff->family = B_MISC_FORMAT_FAMILY;
	mff->version = 100;
	strcpy(mff->mime_type, "audio/x-au");
	strcpy(mff->file_extension, "au");
	strcpy(mff->short_name,  "Sun audio file");
	strcpy(mff->pretty_name, "Sun audio file");
}


status_t
auReader::AllocateCookie(int32 streamNumber, void **cookie)
{
	return B_OK;
}

status_t
auReader::FreeCookie(void *cookie)
{
	return B_OK;
}


status_t
auReader::GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
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
auReader::Seek(void *cookie,
				uint32 seekTo,
				int64 *frame, bigtime_t *time)
{
	int64 pos;

	if (seekTo & B_MEDIA_SEEK_TO_FRAME) {
		if (fRaw)
			pos = (*frame * fBitsPerFrame) / 8;
		else
			pos = (*frame * fDataSize) / fFrameCount;
		pos = (pos / fBlockAlign) * fBlockAlign; // round down to a block start
		TRACE("auReader::Seek to frame %Ld, pos %Ld\n", *frame, pos);
	} else if (seekTo & B_MEDIA_SEEK_TO_TIME) {
		if (fRaw)
			pos = (*time * fFrameRate * fBitsPerFrame) / (1000000LL * 8);
		else
			pos = (*time * fDataSize) / fDuration;
		pos = (pos / fBlockAlign) * fBlockAlign; // round down to a block start
		TRACE("auReader::Seek to time %Ld, pos %Ld\n", *time, pos);
	} else {
		return B_ERROR;
	}

	if (fRaw)
		*frame = (8 * pos) / fBitsPerFrame;
	else
		*frame = (pos * fFrameCount) / fDataSize;
	*time = (*frame * 1000000LL) / fFrameRate;

	TRACE("auReader::Seek newtime %Ld\n", *time);
	TRACE("auReader::Seek newframe %Ld\n", *frame);
	
	if (pos < 0 || pos > fDataSize) {
		TRACE("auReader::Seek invalid position %Ld\n", pos);
		return B_ERROR;
	}
	
	fPosition = pos;
	return B_OK;
}


status_t
auReader::GetNextChunk(void *cookie,
						const void **chunkBuffer, size_t *chunkSize,
						media_header *mediaHeader)
{
	// XXX it might be much better to not return any start_time information for encoded formats here,
	// XXX and instead use the last time returned from seek and count forward after decoding.
	mediaHeader->start_time = (((8 * fPosition) / fBitsPerFrame) * 1000000LL) / fFrameRate;
	mediaHeader->file_pos = fDataStart + fPosition;

	int64 maxreadsize = fDataSize - fPosition;
	int32 readsize = fBufferSize;
	if (maxreadsize < readsize)
		readsize = maxreadsize;
	if (readsize == 0)
		return B_LAST_BUFFER_ERROR;
		
	if (readsize != Source()->ReadAt(fDataStart + fPosition, fBuffer, readsize)) {
		TRACE("auReader::GetNextChunk: unexpected read error\n");
		return B_ERROR;
	}
	
	// XXX if the stream has more than two channels, we need to reorder channel data here
	
	fPosition += readsize;
	*chunkBuffer = fBuffer;
	*chunkSize = readsize;
	return B_OK;
}


Reader *
auReaderPlugin::NewReader()
{
	return new auReader;
}


MediaPlugin *instantiate_plugin()
{
	return new auReaderPlugin;
}
