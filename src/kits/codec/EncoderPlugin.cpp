/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Copyright 2004, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "EncoderPlugin.h"

#include <stdio.h>
#include <string.h>

#include <MediaFormats.h>


Encoder::Encoder()
	:
	fChunkWriter(NULL),
	fMediaPlugin(NULL)
{
}


Encoder::~Encoder()
{
	delete fChunkWriter;
}


// #pragma mark - Convenience stubs


status_t
Encoder::AddTrackInfo(uint32 code, const void* data, size_t size, uint32 flags)
{
	return B_NOT_SUPPORTED;
}


BView*
Encoder::ParameterView()
{
	return NULL;
}


BParameterWeb*
Encoder::ParameterWeb()
{
	return NULL;
}


status_t
Encoder::GetParameterValue(int32 id, void* value, size_t* size) const
{
	return B_NOT_SUPPORTED;
}


status_t
Encoder::SetParameterValue(int32 id, const void* value, size_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
Encoder::GetEncodeParameters(encode_parameters* parameters) const
{
	return B_NOT_SUPPORTED;
}


status_t
Encoder::SetEncodeParameters(encode_parameters* parameters)
{
	return B_NOT_SUPPORTED;
}


// #pragma mark -


status_t
Encoder::WriteChunk(const void* chunkBuffer, size_t chunkSize,
	media_encode_info* encodeInfo)
{
	return fChunkWriter->WriteChunk(chunkBuffer, chunkSize, encodeInfo);
}


void
Encoder::SetChunkWriter(ChunkWriter* writer)
{
	delete fChunkWriter;
	fChunkWriter = writer;
}


// #pragma mark - FBC padding


status_t
Encoder::Perform(perform_code code, void* data)
{
	return B_OK;
}


void Encoder::_ReservedEncoder1() {}
void Encoder::_ReservedEncoder2() {}
void Encoder::_ReservedEncoder3() {}
void Encoder::_ReservedEncoder4() {}
void Encoder::_ReservedEncoder5() {}
void Encoder::_ReservedEncoder6() {}
void Encoder::_ReservedEncoder7() {}
void Encoder::_ReservedEncoder8() {}
void Encoder::_ReservedEncoder9() {}
void Encoder::_ReservedEncoder10() {}
void Encoder::_ReservedEncoder11() {}
void Encoder::_ReservedEncoder12() {}
void Encoder::_ReservedEncoder13() {}
void Encoder::_ReservedEncoder14() {}
void Encoder::_ReservedEncoder15() {}
void Encoder::_ReservedEncoder16() {}
void Encoder::_ReservedEncoder17() {}
void Encoder::_ReservedEncoder18() {}
void Encoder::_ReservedEncoder19() {}
void Encoder::_ReservedEncoder20() {}


//	#pragma mark - EncoderPlugin


EncoderPlugin::EncoderPlugin()
{
}
