/*
 * Copyright 2009-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PackageReader.h>

#include <new>

#include <package/hpkg/ErrorOutput.h>

#include <package/hpkg/PackageFileHeapReader.h>
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
BPackageReader::Init(const char* fileName, uint32 flags)
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->Init(fileName, flags);
}


status_t
BPackageReader::Init(int fd, bool keepFD, uint32 flags)
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->Init(fd, keepFD, flags);
}


status_t
BPackageReader::Init(BPositionIO* file, bool keepFile, uint32 flags)
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->Init(file, keepFile, flags);
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


BPositionIO*
BPackageReader::PackageFile() const
{
	if (fImpl == NULL)
		return NULL;

	return fImpl->PackageFile();
}



BAbstractBufferedDataReader*
BPackageReader::HeapReader() const
{
	return fImpl != NULL ? fImpl->HeapReader() : NULL;
}


}	// namespace BHPKG

}	// namespace BPackageKit
