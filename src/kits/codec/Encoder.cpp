/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Copyright 2004, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Encoder.h>

#include <MediaFormats.h>

#include <stdio.h>
#include <string.h>


namespace BCodecKit {


BEncoder::BEncoder()
	:
	fChunkWriter(NULL),
	fMediaPlugin(NULL)
{
}


BEncoder::~BEncoder()
{
	delete fChunkWriter;
}


// #pragma mark - Convenience stubs


status_t
BEncoder::AddTrackInfo(uint32 code, const void* data, size_t size, uint32 flags)
{
	return B_NOT_SUPPORTED;
}


BView*
BEncoder::ParameterView()
{
	return NULL;
}


BParameterWeb*
BEncoder::ParameterWeb()
{
	return NULL;
}


status_t
BEncoder::GetParameterValue(int32 id, void* value, size_t* size) const
{
	return B_NOT_SUPPORTED;
}


status_t
BEncoder::SetParameterValue(int32 id, const void* value, size_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
BEncoder::GetEncodeParameters(encode_parameters* parameters) const
{
	return B_NOT_SUPPORTED;
}


status_t
BEncoder::SetEncodeParameters(encode_parameters* parameters)
{
	return B_NOT_SUPPORTED;
}


// #pragma mark -


status_t
BEncoder::WriteChunk(const void* chunkBuffer, size_t chunkSize,
	media_encode_info* encodeInfo)
{
	return fChunkWriter->WriteChunk(chunkBuffer, chunkSize, encodeInfo);
}


void
BEncoder::SetChunkWriter(BChunkWriter* writer)
{
	delete fChunkWriter;
	fChunkWriter = writer;
}


// #pragma mark - FBC padding


status_t
BEncoder::Perform(perform_code code, void* data)
{
	return B_OK;
}


void BEncoder::_ReservedEncoder1() {}
void BEncoder::_ReservedEncoder2() {}
void BEncoder::_ReservedEncoder3() {}
void BEncoder::_ReservedEncoder4() {}
void BEncoder::_ReservedEncoder5() {}
void BEncoder::_ReservedEncoder6() {}
void BEncoder::_ReservedEncoder7() {}
void BEncoder::_ReservedEncoder8() {}
void BEncoder::_ReservedEncoder9() {}
void BEncoder::_ReservedEncoder10() {}
void BEncoder::_ReservedEncoder11() {}
void BEncoder::_ReservedEncoder12() {}
void BEncoder::_ReservedEncoder13() {}
void BEncoder::_ReservedEncoder14() {}
void BEncoder::_ReservedEncoder15() {}
void BEncoder::_ReservedEncoder16() {}
void BEncoder::_ReservedEncoder17() {}
void BEncoder::_ReservedEncoder18() {}
void BEncoder::_ReservedEncoder19() {}
void BEncoder::_ReservedEncoder20() {}


//	#pragma mark - EncoderPlugin


BEncoderPlugin::BEncoderPlugin()
{
}


BChunkWriter::BChunkWriter()
{
}


BChunkWriter::~BChunkWriter()
{
}


} // namespace BCodecKit
