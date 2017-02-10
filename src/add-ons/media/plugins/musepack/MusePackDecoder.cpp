/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <stdio.h>
#include <string.h>
#include "MusePackDecoder.h"
#include "mpc/in_mpc.h"


MusePackDecoder::MusePackDecoder()
	:
	fDecoder(NULL)
{
}


MusePackDecoder::~MusePackDecoder()
{
	delete fDecoder;
}


void
MusePackDecoder::GetCodecInfo(media_codec_info *info)
{
	strcpy(info->short_name, "musepack");
	strcpy(info->pretty_name, "MusePack audio codec based on mpcdec by Andree Buschmann");
}


status_t 
MusePackDecoder::Setup(media_format *ioEncodedFormat, const void *infoBuffer, size_t infoSize)
{
	if (infoBuffer == NULL || infoSize != sizeof(MPC_decoder))
		return B_BAD_VALUE;

	fDecoder = (MPC_decoder *)infoBuffer;
	fFrameRate = fDecoder->fInfo->simple.SampleFreq;
	fFramePosition = 0;
	return B_OK;
}


status_t 
MusePackDecoder::NegotiateOutputFormat(media_format *format)
{
	// ToDo: rework the underlying MPC engine to be more flexible

	format->type = B_MEDIA_RAW_AUDIO;
	format->u.raw_audio.frame_rate = fFrameRate;
	format->u.raw_audio.channel_count = fDecoder->fInfo->simple.Channels;
	format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	format->u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
	format->u.raw_audio.buffer_size = sizeof(MPC_SAMPLE_FORMAT) * FRAMELEN * 2;
		// the buffer size is hardcoded in the MPC engine...

	if (format->u.raw_audio.channel_mask == 0) {
		format->u.raw_audio.channel_mask = (format->u.raw_audio.channel_count == 1) ?
			B_CHANNEL_LEFT : B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
	}

	return B_OK;
}


status_t 
MusePackDecoder::SeekedTo(int64 frame, bigtime_t time)
{
	fFramePosition = 0;
	return B_ERROR;
}


status_t 
MusePackDecoder::Decode(void *buffer, int64 *_frameCount, media_header *mediaHeader, media_decode_info *info)
{
	// ToDo: add error checking (check for broken frames)

	mediaHeader->start_time = (bigtime_t) ((1000 * fFramePosition) / (fFrameRate / 1000.0));

//	printf("MusePackDecoder::Decode, start-time %.3f, played-frames %Ld\n", mediaHeader->start_time / 1000000.0, fFramePosition);

	uint32 ring = fDecoder->Zaehler;
	int frameCount = fDecoder->DECODE((MPC_SAMPLE_FORMAT *)buffer);
	frameCount /= 2;
	fDecoder->UpdateBuffer(ring);

	fFramePosition += frameCount;
	*_frameCount = frameCount;

	return frameCount > 0 ? B_OK : B_LAST_BUFFER_ERROR;
}

