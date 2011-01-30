/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageFile.h"

#include <algorithm>
#include <new>

#include <fs_cache.h>
#include <util/AutoLock.h>

#include <package/hpkg/DataOutput.h>
#include <package/hpkg/DataReader.h>
#include <package/hpkg/PackageDataReader.h>

#include "DebugSupport.h"
#include "GlobalFactory.h"
#include "Package.h"


using namespace BPackageKit::BHPKG;


// #pragma mark - DataAccessor


struct PackageFile::IORequestOutput : BDataOutput {
public:
	IORequestOutput(io_request* request)
		:
		fRequest(request)
	{
	}

	virtual status_t WriteData(const void* buffer, size_t size)
	{
		RETURN_ERROR(write_to_io_request(fRequest, buffer, size));
	}

private:
	io_request*	fRequest;
};


struct PackageFile::DataAccessor {
	DataAccessor(BPackageData* data)
		:
		fData(data),
		fDataReader(NULL),
		fReader(NULL),
		fFileCache(NULL)
	{
		mutex_init(&fLock, "file data accessor");
	}

	~DataAccessor()
	{
		file_cache_delete(fFileCache);
		delete fReader;
		delete fDataReader;
		mutex_destroy(&fLock);
	}

	status_t Init(dev_t deviceID, ino_t nodeID, int fd)
	{
		// create a BDataReader for the compressed data
		if (fData->IsEncodedInline()) {
			fDataReader = new(std::nothrow) BBufferDataReader(
				fData->InlineData(), fData->CompressedSize());
		} else
			fDataReader = new(std::nothrow) BFDDataReader(fd);

		if (fDataReader == NULL)
			RETURN_ERROR(B_NO_MEMORY);

		// create a BPackageDataReader
		status_t error = GlobalFactory::Default()->CreatePackageDataReader(
			fDataReader, *fData, fReader);
		if (error != B_OK)
			RETURN_ERROR(error);

		// create a file cache
		fFileCache = file_cache_create(deviceID, nodeID,
			fData->UncompressedSize());
		if (fFileCache == NULL)
			RETURN_ERROR(B_NO_MEMORY);

		return B_OK;
	}

	status_t ReadData(off_t offset, void* buffer, size_t* bufferSize)
	{
		if (offset < 0 || (uint64)offset > fData->UncompressedSize())
			return B_BAD_VALUE;

		*bufferSize = std::min((uint64)*bufferSize,
			fData->UncompressedSize() - offset);

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
			MutexLocker locker(fLock);
			status_t error = fReader->ReadDataToOutput(offset, toRead, &output);
			if (error != B_OK)
				RETURN_ERROR(error);
		}

		return B_OK;
	}

private:
	mutex				fLock;
	BPackageData*		fData;
	BDataReader*			fDataReader;
	BPackageDataReader*	fReader;
	void*				fFileCache;
};


// #pragma mark - PackageFile


PackageFile::PackageFile(Package* package, mode_t mode, const BPackageData& data)
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
	// open the package
	int fd = fPackage->Open();
	if (fd < 0)
		RETURN_ERROR(fd);
	PackageCloser packageCloser(fPackage);

	// create the data accessor
	fDataAccessor = new(std::nothrow) DataAccessor(&fData);
	if (fDataAccessor == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	status_t error = fDataAccessor->Init(deviceID, nodeID, fd);
	if (error != B_OK) {
		delete fDataAccessor;
		fDataAccessor = NULL;
		return error;
	}

	packageCloser.Detach();
	return B_OK;
}


void
PackageFile::VFSUninit()
{
	if (fDataAccessor != NULL) {
		fPackage->Close();
		delete fDataAccessor;
		fDataAccessor = NULL;
	}
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
