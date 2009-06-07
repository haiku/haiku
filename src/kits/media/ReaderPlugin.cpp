/* 
 * Copyright 2004, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ReaderPlugin.h"

#include <stdio.h>


Reader::Reader()
	:
	fSource(0),
	fMediaPlugin(NULL)
{
}


Reader::~Reader()
{
}


status_t
Reader::Seek(void* cookie, uint32 flags, int64* frame, bigtime_t* time)
{
	return B_NOT_SUPPORTED;
}


status_t
Reader::FindKeyFrame(void* cookie, uint32 flags, int64* frame, bigtime_t* time)
{
	return B_NOT_SUPPORTED;
}


BDataIO*
Reader::Source() const
{
	return fSource;
}


void
Reader::Setup(BDataIO *source)
{
	fSource = source;
}


status_t
Reader::Perform(perform_code code, void* _data)
{
	return B_OK;
}

void Reader::_ReservedReader1() {}
void Reader::_ReservedReader2() {}
void Reader::_ReservedReader3() {}
void Reader::_ReservedReader4() {}
void Reader::_ReservedReader5() {}

