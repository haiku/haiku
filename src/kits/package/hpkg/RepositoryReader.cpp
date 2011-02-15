/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/RepositoryReader.h>

#include <new>

#include <package/hpkg/ErrorOutput.h>
#include <package/hpkg/RepositoryContentHandler.h>
#include <package/hpkg/RepositoryReaderImpl.h>


namespace BPackageKit {

namespace BHPKG {


BRepositoryReader::BRepositoryReader(BErrorOutput* errorOutput)
	:
	fImpl(new (std::nothrow) RepositoryReaderImpl(errorOutput))
{
}


BRepositoryReader::~BRepositoryReader()
{
	delete fImpl;
}


status_t
BRepositoryReader::Init(const char* fileName)
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->Init(fileName);
}


status_t
BRepositoryReader::ParseContent(BRepositoryContentHandler* contentHandler)
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->ParseContent(contentHandler);
}


}	// namespace BHPKG

}	// namespace BPackageKit
