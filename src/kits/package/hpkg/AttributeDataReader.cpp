/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/DataReader.h>

#include <errno.h>

#include <fs_attr.h>


namespace BPackageKit {

namespace BHPKG {


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


}	// namespace BHPKG

}	// namespace BPackageKit
