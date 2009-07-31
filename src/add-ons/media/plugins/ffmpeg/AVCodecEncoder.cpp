/*
 * Copyright 2009, Stephan Amßus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "AVCodecEncoder.h"

#include <stdio.h>


#undef TRACE
#define TRACE_AV_CODEC_ENCODER
#ifdef TRACE_AV_CODEC_ENCODER
#	define TRACE(x...)	printf(x)
#else
#	define TRACE(x...)
#endif


AVCodecEncoder::AVCodecEncoder(const char* shortName)
	:
	Encoder()
{
	TRACE("AVCodecEncoder::AVCodecEncoder()\n");
}


AVCodecEncoder::~AVCodecEncoder()
{
	TRACE("AVCodecEncoder::~AVCodecEncoder()\n");
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

	if (inputFormat == NULL)
		return B_BAD_VALUE;

	return B_NOT_SUPPORTED;
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

	return B_NOT_SUPPORTED;
}
