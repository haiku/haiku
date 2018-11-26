/*
 * Copyright 2009-2010, Stephan Aßmus <superstippi@gmx.de>.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2004, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Reader.h>

#include <stdio.h>


namespace BCodecKit {


BReader::BReader()
	:
	fSource(NULL),
	fMediaPlugin(NULL)
{
}


BReader::~BReader()
{
}


status_t
BReader::GetMetaData(BMetaData* data)
{
	return B_NOT_SUPPORTED;
}


status_t
BReader::Seek(void* cookie, uint32 flags, int64* frame, bigtime_t* time)
{
	return B_NOT_SUPPORTED;
}


status_t
BReader::FindKeyFrame(void* cookie, uint32 flags, int64* frame, bigtime_t* time)
{
	return B_NOT_SUPPORTED;
}


status_t
BReader::GetStreamMetaData(void* cookie, BMetaData* data)
{
	return B_NOT_SUPPORTED;
}


BDataIO*
BReader::Source() const
{
	return fSource;
}


void
BReader::_Setup(BDataIO *source)
{
	fSource = source;
}


status_t
BReader::Perform(perform_code code, void* _data)
{
	return B_OK;
}


void BReader::_ReservedReader1() {}
void BReader::_ReservedReader2() {}
void BReader::_ReservedReader3() {}
void BReader::_ReservedReader4() {}
void BReader::_ReservedReader5() {}


} // namespace BCodecKit
