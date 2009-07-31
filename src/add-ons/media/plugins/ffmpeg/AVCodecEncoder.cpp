/*
 * Copyright 2009, Stephan Amßus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "AVCodecEncoder.h"

#include <new>

#include <stdio.h>

extern "C" {
	#include "rational.h"
}


#undef TRACE
#define TRACE_AV_CODEC_ENCODER
#ifdef TRACE_AV_CODEC_ENCODER
#	define TRACE(x...)	printf(x)
#else
#	define TRACE(x...)
#endif


static const size_t kDefaultChunkBufferSize = FF_MIN_BUFFER_SIZE;


AVCodecEncoder::AVCodecEncoder(uint32 codecID)
	:
	Encoder(),
	fCodec(NULL),
	fContext(avcodec_alloc_context()),
	fInputPicture(avcodec_alloc_frame()),
//	fOutputPicture(avcodec_alloc_frame()),
	fCodecInitDone(false),
	fChunkBuffer(new(std::nothrow) uint8[kDefaultChunkBufferSize])
{
	TRACE("AVCodecEncoder::AVCodecEncoder()\n");

	fCodec = avcodec_find_encoder((enum CodecID)codecID);
	TRACE("  found AVCodec: %p\n", fCodec);

	memset(&fInputFormat, 0, sizeof(media_format));
}


AVCodecEncoder::~AVCodecEncoder()
{
	TRACE("AVCodecEncoder::~AVCodecEncoder()\n");

	if (fCodecInitDone)
		avcodec_close(fContext);

//	free(fOutputPicture);
	free(fInputPicture);
	free(fContext);

	delete[] fChunkBuffer;
}


status_t
AVCodecEncoder::AcceptedFormat(const media_format* proposedInputFormat,
	media_format* _acceptedInputFormat)
{
	TRACE("AVCodecEncoder::AcceptedFormat(%p, %p)\n", proposedInputFormat,
		_acceptedInputFormat);

	if (proposedInputFormat == NULL)
		return B_BAD_VALUE;

	if (_acceptedInputFormat != NULL) {
		memcpy(_acceptedInputFormat, proposedInputFormat,
			sizeof(media_format));
	}

	return B_OK;
}


status_t
AVCodecEncoder::SetUp(const media_format* inputFormat)
{
	TRACE("AVCodecEncoder::SetUp()\n");

	if (fContext == NULL || fCodec == NULL)
		return B_NO_INIT;

	if (inputFormat == NULL)
		return B_BAD_VALUE;

	fInputFormat = *inputFormat;

	if (fInputFormat.type == B_MEDIA_RAW_VIDEO) {
		fContext->width = fInputFormat.u.raw_video.display.line_width;
		fContext->height = fInputFormat.u.raw_video.display.line_count;
//		fContext->gop_size = 12;
		fContext->pix_fmt = PIX_FMT_BGR32;
//		fContext->rate_emu = 0;
		// TODO: Setup rate control:
//		fContext->rc_eq = NULL;
//		fContext->rc_max_rate = 0;
//		fContext->rc_min_rate = 0;
		fContext->sample_aspect_ratio.num
			= fInputFormat.u.raw_video.pixel_width_aspect;
		fContext->sample_aspect_ratio.den
			= fInputFormat.u.raw_video.pixel_height_aspect;
		if (fContext->sample_aspect_ratio.num == 0
			|| fContext->sample_aspect_ratio.den == 0) {
			av_reduce(&fContext->sample_aspect_ratio.num,
				&fContext->sample_aspect_ratio.den, fContext->width,
				fContext->height, 256);
		}

		// TODO: This should already happen in AcceptFormat()
		fInputFormat.u.raw_video.display.bytes_per_row = fContext->width * 4;
	} else {
		return B_NOT_SUPPORTED;
	}

	// Open the codec
	int result = avcodec_open(fContext, fCodec);
	fCodecInitDone = (result >= 0);

	TRACE("  avcodec_open(): %d\n", result);

	return fCodecInitDone ? B_OK : B_ERROR;
}


status_t
AVCodecEncoder::GetEncodeParameters(encode_parameters* parameters) const
{
	TRACE("AVCodecEncoder::GetEncodeParameters(%p)\n", parameters);

	return B_NOT_SUPPORTED;
}


status_t
AVCodecEncoder::SetEncodeParameters(encode_parameters* parameters) const
{
	TRACE("AVCodecEncoder::SetEncodeParameters(%p)\n", parameters);

	return B_NOT_SUPPORTED;
}


status_t
AVCodecEncoder::Encode(const void* buffer, int64 frameCount,
	media_encode_info* info)
{
	TRACE("AVCodecEncoder::Encode(%p, %lld, %p)\n", buffer, frameCount, info);

	if (fInputFormat.type == B_MEDIA_RAW_AUDIO)
		return _EncodeAudio(buffer, frameCount, info);
	else if (fInputFormat.type == B_MEDIA_RAW_VIDEO)
		return _EncodeVideo(buffer, frameCount, info);
	else
		return B_NO_INIT;
}


// #pragma mark -


status_t
AVCodecEncoder::_EncodeAudio(const void* buffer, int64 frameCount,
	media_encode_info* info)
{
	TRACE("AVCodecEncoder::_EncodeAudio(%p, %lld, %p)\n", buffer, frameCount,
		info);

	return B_NOT_SUPPORTED;
}


status_t
AVCodecEncoder::_EncodeVideo(const void* buffer, int64 frameCount,
	media_encode_info* info)
{
	TRACE("AVCodecEncoder::_EncodeVideo(%p, %lld, %p)\n", buffer, frameCount,
		info);

	if (fChunkBuffer == NULL)
		return B_NO_MEMORY;

	status_t ret = B_OK;

	while (frameCount > 0) {
		size_t bpr = fInputFormat.u.raw_video.display.bytes_per_row;
		size_t bufferSize = fInputFormat.u.raw_video.display.line_count * bpr;

		fInputPicture->data[0] = (uint8_t*)buffer;
		fInputPicture->linesize[0] = bpr;

		int usedBytes = avcodec_encode_video(fContext, fChunkBuffer,
			kDefaultChunkBufferSize, fInputPicture);

		if (usedBytes < 0) {
			TRACE("  avcodec_encode_video() failed: %d\n", usedBytes);
			return B_ERROR;
		}

		// Write the chunk
		ret = WriteChunk(fChunkBuffer, usedBytes, info);
		if (ret != B_OK)
			break;

		// Skip to the next frame (but usually, there is only one to encode
		// for video).
		frameCount--;
		buffer = (const void*)((const uint8*)buffer + bufferSize);
	}

	return ret;
}

