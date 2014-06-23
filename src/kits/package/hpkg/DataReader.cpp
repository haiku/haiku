/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/DataReader.h>

#include <DataIO.h>

#include <string.h>


namespace BPackageKit {

namespace BHPKG {


// #pragma mark - BDataReader


BDataReader::~BDataReader()
{
}


// #pragma mark - BAbstractBufferedDataReader


BAbstractBufferedDataReader::~BAbstractBufferedDataReader()
{
}


status_t
BAbstractBufferedDataReader::ReadData(off_t offset, void* buffer, size_t size)
{
	BMemoryIO output(buffer, size);
	return ReadDataToOutput(offset, size, &output);
}


// #pragma mark - BBufferDataReader


BBufferDataReader::BBufferDataReader(const void* data, size_t size)
	:
	fData(data),
	fSize(size)
{
}


status_t
BBufferDataReader::ReadData(off_t offset, void* buffer, size_t size)
{
	if (size == 0)
		return B_OK;

	if (offset < 0)
		return B_BAD_VALUE;

	if (size > fSize || offset > (off_t)fSize - (off_t)size)
		return B_ERROR;

	memcpy(buffer, (const uint8*)fData + offset, size);
	return B_OK;
}


status_t
BBufferDataReader::ReadDataToOutput(off_t offset, size_t size, BDataIO* output)
{
	if (size == 0)
		return B_OK;

	if (offset < 0)
		return B_BAD_VALUE;

	if (size > fSize || offset > (off_t)fSize - (off_t)size)
		return B_ERROR;

	return output->WriteExactly((const uint8*)fData + offset, size);
}


}	// namespace BHPKG

}	// namespace BPackageKit
