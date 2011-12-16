/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/DataReader.h>

#include <errno.h>
#include <unistd.h>


namespace BPackageKit {

namespace BHPKG {


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


}	// namespace BHPKG

}	// namespace BPackageKit
