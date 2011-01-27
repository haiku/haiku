/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/DataOutput.h>

#include <string.h>


namespace BPackageKit {

namespace BHaikuPackage {

namespace BPrivate {


// #pragma mark - DataOutput


DataOutput::~DataOutput()
{
}


// #pragma mark - BufferDataOutput


BufferDataOutput::BufferDataOutput(void* buffer, size_t size)
	:
	fBuffer(buffer),
	fSize(size),
	fBytesWritten(0)
{
}


status_t
BufferDataOutput::WriteData(const void* buffer, size_t size)
{
	if (size == 0)
		return B_OK;
	if (size > fSize - fBytesWritten)
		return B_BAD_VALUE;

	memcpy((uint8*)fBuffer + fBytesWritten, buffer, size);
	fBytesWritten += size;

	return B_OK;
}


}	// namespace BPrivate

}	// namespace BHaikuPackage

}	// namespace BPackageKit
