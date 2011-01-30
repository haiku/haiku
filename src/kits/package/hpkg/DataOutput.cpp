/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/DataOutput.h>

#include <string.h>


namespace BPackageKit {

namespace BHPKG {


// #pragma mark - BDataOutput


BDataOutput::~BDataOutput()
{
}


// #pragma mark - BBufferDataOutput


BBufferDataOutput::BBufferDataOutput(void* buffer, size_t size)
	:
	fBuffer(buffer),
	fSize(size),
	fBytesWritten(0)
{
}


status_t
BBufferDataOutput::WriteData(const void* buffer, size_t size)
{
	if (size == 0)
		return B_OK;
	if (size > fSize - fBytesWritten)
		return B_BAD_VALUE;

	memcpy((uint8*)fBuffer + fBytesWritten, buffer, size);
	fBytesWritten += size;

	return B_OK;
}


}	// namespace BHPKG

}	// namespace BPackageKit
