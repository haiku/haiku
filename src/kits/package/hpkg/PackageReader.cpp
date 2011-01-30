/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PackageReader.h>

#include <new>

#include <package/hpkg/ErrorOutput.h>
#include <package/hpkg/PackageReaderImpl.h>


namespace BPackageKit {

namespace BHPKG {


BPackageReader::BPackageReader(BErrorOutput* errorOutput)
	:
	fImpl(new (std::nothrow) PackageReaderImpl(errorOutput))
{
}


BPackageReader::~BPackageReader()
{
	delete fImpl;
}


status_t
BPackageReader::Init(const char* fileName)
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->Init(fileName);
}


status_t
BPackageReader::Init(int fd, bool keepFD)
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->Init(fd, keepFD);
}


status_t
BPackageReader::ParseContent(BPackageContentHandler* contentHandler)
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->ParseContent(contentHandler);
}


status_t
BPackageReader::ParseContent(BLowLevelPackageContentHandler* contentHandler)
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->ParseContent(contentHandler);
}


int
BPackageReader::PackageFileFD()
{
	if (fImpl == NULL)
		return -1;

	return fImpl->PackageFileFD();
}


}	// namespace BHPKG

}	// namespace BPackageKit
