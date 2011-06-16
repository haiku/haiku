/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/DataReader.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <fs_attr.h>


namespace BPackageKit {

namespace BHPKG {


// #pragma mark - BDataReader


BDataReader::~BDataReader()
{
}


// #pragma mark - BFDDataReader


BFDDataReader::BFDDataReader(int fd)
	:
	fFD(fd)
{
}


status_t
BFDDataReader::ReadData(off_t offset, void* buffer, size_t size)
{
	ssize_t bytesRead = pread(fFD, buffer, size, offset);
	if (bytesRead < 0)
		return errno;
	return (size_t)bytesRead == size ? B_OK : B_ERROR;
}


// #pragma mark - BAttributeDataReader


BAttributeDataReader::BAttributeDataReader(int fd, const char* attribute,
	uint32 type)
	:
	fFD(fd),
	fType(type),
	fAttribute(attribute)
{
}


status_t
BAttributeDataReader::ReadData(off_t offset, void* buffer, size_t size)
{
	ssize_t bytesRead = fs_read_attr(fFD, fAttribute, fType, offset, buffer,
		size);
	if (bytesRead < 0)
		return errno;
	return (size_t)bytesRead == size ? B_OK : B_ERROR;
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


}	// namespace BHPKG

}	// namespace BPackageKit
