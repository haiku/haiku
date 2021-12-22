/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageFile.h"

#include <algorithm>
#include <new>

#include <fs_cache.h>

#include <DataIO.h>
#include <package/hpkg/DataReader.h>
#include <package/hpkg/PackageDataReader.h>

#include <AutoDeleter.h>
#include <util/AutoLock.h>

#include "DebugSupport.h"
#include "ClassCache.h"
#include "Package.h"


using namespace BPackageKit::BHPKG;


// #pragma mark - class cache


CLASS_CACHE(PackageFile);


// #pragma mark - DataAccessor


struct PackageFile::IORequestOutput : BDataIO {
public:
	IORequestOutput(io_request* request)
		:
		fRequest(request)
	{
	}

	virtual ssize_t Write(const void* buffer, size_t size)
	{
		status_t error = write_to_io_request(fRequest, buffer, size);
		RETURN_ERROR(error == B_OK ? (ssize_t)size : (ssize_t)error);
	}

private:
	io_request*	fRequest;
};


struct PackageFile::DataAccessor {
	DataAccessor(Package* package, PackageData* data)
		:
		fPackage(package),
		fData(data),
		fReader(NULL),
		fFileCache(NULL)
	{
		mutex_init(&fLock, "file data accessor");
	}

	~DataAccessor()
	{
		file_cache_delete(fFileCache);
		delete fReader;
		mutex_destroy(&fLock);
	}

	status_t Init(dev_t deviceID, ino_t nodeID, int fd)
	{
		// create a reader for the data
		status_t error = fPackage->CreateDataReader(*fData, fReader);
		if (error != B_OK)
			return error;

		// create a file cache
		fFileCache = file_cache_create(deviceID, nodeID,
			fData->UncompressedSize());
		if (fFileCache == NULL)
			RETURN_ERROR(B_NO_MEMORY);

		return B_OK;
	}

	status_t ReadData(off_t offset, void* buffer, size_t* bufferSize)
	{
		return file_cache_read(fFileCache, NULL, offset, buffer, bufferSize);
	}

	status_t ReadData(io_request* request)
	{
		off_t offset = io_request_offset(request);
		size_t size = io_request_length(request);

		if (offset < 0 || (uint64)offset > fData->UncompressedSize())
			RETURN_ERROR(B_BAD_VALUE);

		size_t toRead = std::min((uint64)size,
			fData->UncompressedSize() - offset);

		if (toRead > 0) {
			IORequestOutput output(request);
			MutexLocker locker(fLock, false, fData->Version() == 1);
				// V2 readers are reentrant
			status_t error = fReader->ReadDataToOutput(offset, toRead, &output);
			if (error != B_OK)
				RETURN_ERROR(error);
		}

		return B_OK;
	}

private:
	mutex							fLock;
	Package*						fPackage;
	PackageData*					fData;
	BAbstractBufferedDataReader*	fReader;
	void*							fFileCache;
};


// #pragma mark - PackageFile


PackageFile::PackageFile(Package* package, mode_t mode, const PackageData& data)
	:
	PackageLeafNode(package, mode),
	fData(data),
	fDataAccessor(NULL)
{
}


PackageFile::~PackageFile()
{
}


status_t
PackageFile::VFSInit(dev_t deviceID, ino_t nodeID)
{
	status_t error = PackageNode::VFSInit(deviceID, nodeID);
	if (error != B_OK)
		return error;
	MethodDeleter<PackageNode, void, &PackageNode::NonVirtualVFSUninit>
		baseClassUninit(this);

	// open the package -- that's already done by PackageNode::VFSInit(), so it
	// shouldn't fail here. We only need to do it again, since we need the FD.
	BReference<Package> package(GetPackage());
	int fd = package->Open();
	if (fd < 0)
		RETURN_ERROR(fd);
	PackageCloser packageCloser(package);

	// create the data accessor
	fDataAccessor = new(std::nothrow) DataAccessor(package, &fData);
	if (fDataAccessor == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	error = fDataAccessor->Init(deviceID, nodeID, fd);
	if (error != B_OK) {
		delete fDataAccessor;
		fDataAccessor = NULL;
		return error;
	}

	baseClassUninit.Detach();
	return B_OK;
}


void
PackageFile::VFSUninit()
{
	if (fDataAccessor != NULL) {
		delete fDataAccessor;
		fDataAccessor = NULL;
	}

	PackageNode::VFSUninit();
}


off_t
PackageFile::FileSize() const
{
	return fData.UncompressedSize();
}


status_t
PackageFile::Read(off_t offset, void* buffer, size_t* bufferSize)
{
	if (fDataAccessor == NULL)
		return B_BAD_VALUE;
	return fDataAccessor->ReadData(offset, buffer, bufferSize);
}


status_t
PackageFile::Read(io_request* request)
{
	if (fDataAccessor == NULL)
		return B_BAD_VALUE;
	return fDataAccessor->ReadData(request);
}
