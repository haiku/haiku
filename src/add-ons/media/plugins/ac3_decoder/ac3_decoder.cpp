/*
 *  ac3_decoder.cpp
 *  Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>
 *
 *  This file is part of an AC-3 decoder plugin for the OpenBeOS
 *  media kit. OpenBeOS can be found at http://www.openbeos.org
 *
 *  This file is distributed under the terms of the MIT license.
 *  You can also use it under the terms of the GPL version 2.
 *
 *  Since this file is linked against a GPL licensed library,
 *  the combined work of this file and the liba52 library, the
 *  ac3_decoder plugin must be distributed as GPL licensed.
 */

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "ac3_decoder.h"


#if 0
  #define TRACE printf
#else
  #define TRACE(a...)
#endif


AC3Decoder::AC3Decoder()
 :	fInputBuffer(0)
 ,	fInputBufferSize(0)
 ,	fInputChunk(0)
 ,	fInputChunkSize(0)
 ,	fInputChunkOffset(0)
 ,	fStartTime(0)
 ,	fHasStreamInfo(false)
 ,	fDisableDynamicCompression(false)
 ,	fChannelCount(0)
 ,	fChannelMask(0)
{
	fInputBuffer = malloc(INPUT_BUFFER_MAX_SIZE);
	
	fState = a52_init(0);
	fSamples = a52_samples(fState);
}


AC3Decoder::~AC3Decoder()
{
	free(fInputBuffer);
	a52_free(fState);
}


void
AC3Decoder::GetCodecInfo(media_codec_info *info)
{
	strcpy(info->short_name, "AC-3");
	strcpy(info->pretty_name, "AC-3 audio decoder (liba52)");
}


status_t
AC3Decoder::Setup(media_format *ioEncodedFormat,
				  const void *infoBuffer, int32 infoSize)
{
	return B_OK;
}


status_t
AC3Decoder::NegotiateOutputFormat(media_format *ioDecodedFormat)
{
	if (!fHasStreamInfo)
		fHasStreamInfo = GetStreamInfo();
	if (!fHasStreamInfo) {
		TRACE("AC3Decoder::NegotiateOutputFormat: couldn't get stream info\n");
		return false;
	}

	TRACE("AC3Decoder::NegotiateOutputFormat: fFlags 0x%x, fFrameRate %d, fBitRate %d\n", fFlags, fFrameRate, fBitRate);

	// Sample offsets of the output buffer are stored in fInterleaveOffset
	// When a channel is not present, it is skipped, the rest is shifted left
	// The block of samples order presented by liba52 is
	// 0 = LFE, 1 = left, 2 = center, 3 = right, 4 = left-surround, 5 = right-surround
	// The required sample order in the output buffer is:
	// 0 = B_CHANNEL_LEFT, 1 = B_CHANNEL_RIGHT, 2 = B_CHANNEL_CENTER, 3 = B_CHANNEL_SUB
	// 4 = B_CHANNEL_REARLEFT, 5 = B_CHANNEL_REARRIGHT, 6 = B_CHANNEL_BACK_CENTER
	// y = fInterleaveOffset[x] with y = media kit and x = liba52 (beware of non present channels, they are skipped)

	if (fFlags & A52_LFE) {
		fChannelCount = 1;
		fChannelMask = B_CHANNEL_SUB;
	} else {
		fChannelCount = 0;
		fChannelMask = 0;
	}
	fInterleaveOffset = 0;

	switch (fFlags & A52_CHANNEL_MASK) {
		case A52_CHANNEL: // XXX two independant mono channels
		{
			fChannelCount += 2;
			fChannelMask |= B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
			static int lfe_offsets[6] = { 2, 0, 1 };
			static int nrm_offsets[6] = { 0, 1 };
			fInterleaveOffset = (fFlags & A52_LFE) ? lfe_offsets : nrm_offsets;
			break;
		}
		case A52_MONO: {
			fChannelCount += 1;
			fChannelMask |= B_CHANNEL_LEFT;
			static int lfe_offsets[6] = { 1, 0 };
			static int nrm_offsets[6] = { 0 };
			fInterleaveOffset = (fFlags & A52_LFE) ? lfe_offsets : nrm_offsets;
			break;
		}
		case A52_STEREO:
		{
			fChannelCount += 2;
			fChannelMask |= B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
			static int lfe_offsets[6] = { 2, 0, 1 };
			static int nrm_offsets[6] = { 0, 1 };
			fInterleaveOffset = (fFlags & A52_LFE) ? lfe_offsets : nrm_offsets;
			break;
		}
		case A52_3F: // 3 front channels (left, center, right)
		{
			fChannelCount += 3;
			fChannelMask |= B_CHANNEL_LEFT | B_CHANNEL_RIGHT | B_CHANNEL_CENTER;
			static int lfe_offsets[6] = { 3, 0, 2, 1 };
			static int nrm_offsets[6] = { 0, 2, 1 };
			fInterleaveOffset = (fFlags & A52_LFE) ? lfe_offsets : nrm_offsets;
			break;
		}
		case A52_2F1R: // 2 front, 1 rear surround channel (L, R, S)
		{
			fChannelCount += 3;
			fChannelMask |= B_CHANNEL_LEFT | B_CHANNEL_RIGHT | B_CHANNEL_BACK_CENTER;
			static int lfe_offsets[6] = { 2, 0, 3, 1 };
			static int nrm_offsets[6] = { 0, 2, 1 };
			fInterleaveOffset = (fFlags & A52_LFE) ? lfe_offsets : nrm_offsets;
			break;
		}

		case A52_3F1R: // 3 front, 1 rear surround channel (L, C, R, S)
		{
			fChannelCount += 4;
			fChannelMask |= B_CHANNEL_LEFT | B_CHANNEL_RIGHT | B_CHANNEL_CENTER | B_CHANNEL_BACK_CENTER;
			static int lfe_offsets[6] = { 3, 0, 2, 1, 4 };
			static int nrm_offsets[6] = { 0, 2, 1, 3 };
			fInterleaveOffset = (fFlags & A52_LFE) ? lfe_offsets : nrm_offsets;
			break;
		}
		case A52_2F2R: // 2 front, 2 rear surround channels (L, R, LS, RS)
		{
			fChannelCount += 4;
			fChannelMask |= B_CHANNEL_LEFT | B_CHANNEL_RIGHT | B_CHANNEL_REARLEFT | B_CHANNEL_REARRIGHT;
			static int lfe_offsets[6] = { 2, 0, 1, 3, 4};
			static int nrm_offsets[6] = { 0, 1, 2, 3 };
			fInterleaveOffset = (fFlags & A52_LFE) ? lfe_offsets : nrm_offsets;
			break;
		}
		case A52_3F2R: // 3 front, 2 rear surround channels (L, C, R, LS, RS)
		{
			fChannelCount += 5;
			fChannelMask |= B_CHANNEL_LEFT | B_CHANNEL_RIGHT | B_CHANNEL_CENTER | B_CHANNEL_REARLEFT | B_CHANNEL_REARRIGHT;
			static int lfe_offsets[6] = { 3, 0, 2, 1, 4, 5 };
			static int nrm_offsets[6] = { 0, 2, 1, 3, 4 };
			fInterleaveOffset = (fFlags & A52_LFE) ? lfe_offsets : nrm_offsets;
			break;
		}
		case A52_DOLBY:
		{
			fChannelCount += 2;
			fChannelMask |= B_CHANNEL_LEFT | B_CHANNEL_RIGHT;
			static int lfe_offsets[6] = { 2, 0, 1 };
			static int nrm_offsets[6] = { 0, 1 };
			fInterleaveOffset = (fFlags & A52_LFE) ? lfe_offsets : nrm_offsets;
			break;
		}

		default:
			TRACE("AC3Decoder::NegotiateOutputFormat: can't decode channel setup\n");
			return B_ERROR;
	}

	ioDecodedFormat->type = B_MEDIA_RAW_AUDIO;
	ioDecodedFormat->u.raw_audio.frame_rate = fFrameRate;
	ioDecodedFormat->u.raw_audio.channel_count = fChannelCount;
	ioDecodedFormat->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	ioDecodedFormat->u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
	ioDecodedFormat->u.raw_audio.buffer_size = 6 * 256 * fChannelCount * sizeof(float);
	ioDecodedFormat->u.raw_audio.channel_mask = fChannelMask;
	
	return B_OK;
}


status_t
AC3Decoder::Seek(uint32 seekTo,
				 int64 seekFrame, int64 *frame,
				 bigtime_t seekTime, bigtime_t *time)
{
	fInputChunkSize = 0;
	fInputBufferSize = 0;
	return B_OK;
}


status_t
AC3Decoder::Decode(void *buffer, int64 *frameCount,
				   media_header *mediaHeader, media_decode_info *info)
{
//	bigtime_t start = system_time();

	mediaHeader->start_time = fStartTime;
	
//	printf("AC3Decoder::Decode: start time %Ld\n", fStartTime);
	
	set_thread_priority(find_thread(0), 7);

	if (!DecodeNext()) {
		TRACE("AC3Decoder::Decode: DecodeNext failed\n");
		return B_ERROR;
	}
	
	/*
		Every A/52 frame is composed of 6 blocks, each with an output of 256
		samples for each channel. The a52_block() function decodes the next
		block in the frame, and should be called 6 times to decode all of the
		audio in the frame. After each call, you should extract the audio data
		from the sample buffer.
	*/
	
	// fInterleaveOffset[] is used to provide a correct interleave of
	// multi channel audio
	
	for (int i = 0; i < 6; i++) {
		a52_block(fState);
		for (int j = 0; j < fChannelCount; j++) {
			float *buf = (float *) buffer;
			buf += i * 256 * fChannelCount + fInterleaveOffset[j];
			for (int k = 0; k < 256; k++) {
				*buf = fSamples[j * 256 + k];
				buf += fChannelCount;
			}
		}
	}
	
	fStartTime += 6 * 256 * 1000000LL / fFrameRate;
	*frameCount = 6 * 256;

//	printf("AC3Decoder::Decode: finished, %Ld usec\n", system_time() - start);

	return B_OK;
}

bool
AC3Decoder::InputGetData(void **buffer, int size)
{
	TRACE("AC3Decoder::InputGetData: %d bytes enter\n", size);

	for (;;) {
		if (fInputBufferSize >= size) {
			*buffer = fInputBuffer;
			TRACE("AC3Decoder::InputGetData: finished\n");
			return true;
		}
		if (fInputChunkSize > 0) {
			int bytes = min_c(size - fInputBufferSize, fInputChunkSize);
			memcpy((char *)fInputBuffer + fInputBufferSize, (char *)fInputChunk + fInputChunkOffset, bytes);
			fInputChunkOffset += bytes;
			fInputChunkSize -= bytes;
			fInputBufferSize += bytes;
			continue;
		}
		while (fInputChunkSize == 0) {
			fInputChunkOffset = 0;
			media_header mh;
			if (B_OK != GetNextChunk(&fInputChunk, &fInputChunkSize, &mh)) {
				TRACE("AC3Decoder::InputGetData: failed\n");
				return false;
			}
			fStartTime = mh.start_time;
			TRACE("AC3Decoder::InputGetData: got new chunk, %ld bytes, start time %Ld\n", fInputChunkSize, fStartTime);
		}
	}
}

void
AC3Decoder::InputRemoveData(int size)
{
	fInputBufferSize -= size;
	
	if (fInputBufferSize)
		memmove(fInputBuffer, (char *)fInputBuffer + size, fInputBufferSize);
}

bool
AC3Decoder::GetStreamInfo()
{
	for (;;) {
		void *input;
		if (!InputGetData(&input, 7)) {
			TRACE("AC3Decoder::GetStreamInfo: can't get 7 bytes\n");
			return false;
		}
		if (0 != a52_syncinfo((uint8_t *)input, &fFlags, &fFrameRate, &fBitRate))
			return true;
		TRACE("AC3Decoder::GetStreamInfo: a52_syncinfo failed\n");
		InputRemoveData(1);
	}
}

bool
AC3Decoder::DecodeNext()
{
	for (;;) {
		void *input;
		if (!InputGetData(&input, 7)) {
			TRACE("AC3Decoder::DecodeNext: can't get 7 bytes\n");
			return false;
		}
		
		int flags, sample_rate, bit_rate;
		
		int bytes = a52_syncinfo((uint8_t *)input, &flags, &sample_rate, &bit_rate);
		if (bytes == 0) {
			TRACE("AC3Decoder::DecodeNext: syncinfo failed\n");
			InputRemoveData(1);
			continue;
		}

		if (!InputGetData(&input, bytes)) {
			TRACE("AC3Decoder::DecodeNext: can't get %d data bytes\n", bytes);
			return false;
		}
		
//		printf("fFlags 0x%x, flags 0x%x\n", fFlags, flags);
		
		flags = fFlags | A52_ADJUST_LEVEL;
		sample_t level = 1.0;
		if (0 != a52_frame(fState, (uint8_t *)input, &flags, &level, 0)) {
			TRACE("AC3Decoder::DecodeNext: a52_frame failed\n");
			return false;
		}
			
		TRACE("decoded %d bytes, flags 0x%x\n", bytes, fFlags);

		InputRemoveData(bytes);
			
		if (fDisableDynamicCompression)
			a52_dynrng(fState, NULL, 0);
			
		return true;
	}
}


Decoder *
AC3DecoderPlugin::NewDecoder(uint index)
{
	return new AC3Decoder;
}

status_t
AC3DecoderPlugin::GetSupportedFormats(media_format ** formats, size_t * count)
{
	static media_format format;

	format.type = B_MEDIA_ENCODED_AUDIO;
	format.u.encoded_audio = media_encoded_audio_format::wildcard;

	*formats = &format;
	*count = 1;

	media_format_description desc;
	desc.family = B_WAV_FORMAT_FAMILY;
	desc.u.wav.codec = 0x2000;

	BMediaFormats mediaFormats;
	return mediaFormats.MakeFormatFor(&desc, 1, &format);
}


MediaPlugin *instantiate_plugin()
{
	return new AC3DecoderPlugin;
}
