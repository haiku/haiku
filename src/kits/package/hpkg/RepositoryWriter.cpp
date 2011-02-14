/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/RepositoryWriter.h>

#include <new>

#include <package/hpkg/RepositoryWriterImpl.h>
#include <package/RepositoryInfo.h>


namespace BPackageKit {

namespace BHPKG {


BRepositoryWriter::BRepositoryWriter(BRepositoryWriterListener* listener,
	BRepositoryInfo* repositoryInfo)
	:
	fImpl(new (std::nothrow) RepositoryWriterImpl(listener, repositoryInfo))
{
}


BRepositoryWriter::~BRepositoryWriter()
{
	delete fImpl;
}


status_t
BRepositoryWriter::Init(const char* fileName)
{
	if (fImpl == NULL)
		return B_NO_MEMORY;

	return fImpl->Init(fileName);
}


status_t
BRepositoryWriter::AddPackage(const BEntry& packageEntry)
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->AddPackage(packageEntry);
}


status_t
BRepositoryWriter::Finish()
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->Finish();
}


}	// namespace BHPKG

}	// namespace BPackageKit
