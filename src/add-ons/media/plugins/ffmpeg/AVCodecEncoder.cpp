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
AVCodecEncoder::SetFormat(const media_file_format& fileFormat,
	media_format* _inOutEncodedFormat)
{
	TRACE("AVCodecEncoder::SetFormat()\n");

	return B_NOT_SUPPORTED;
}


status_t
AVCodecEncoder::AddTrackInfo(uint32 code, const void* data, size_t size,
	uint32 flags)
{
	TRACE("AVCodecEncoder::AddTrackInfo(%lu, %p, %ld, %lu)\n", code, data,
		size, flags);

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
