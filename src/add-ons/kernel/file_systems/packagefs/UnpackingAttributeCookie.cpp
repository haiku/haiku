/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "UnpackingAttributeCookie.h"

#include <algorithm>
#include <new>

#include <package/hpkg/DataReader.h>
#include <package/hpkg/PackageData.h>
#include <package/hpkg/PackageDataReader.h>

#include <AutoDeleter.h>

#include "DebugSupport.h"
#include "GlobalFactory.h"
#include "Package.h"
#include "PackageNode.h"


using BPackageKit::BHPKG::BBufferDataReader;
using BPackageKit::BHPKG::BFDDataReader;


static status_t
read_package_data(const BPackageData& data, BDataReader* dataReader,
	off_t offset, void* buffer, size_t* bufferSize)
{
	// create a BPackageDataReader
	BPackageDataReader* reader;
	status_t error = GlobalFactory::Default()->CreatePackageDataReader(
		dataReader, data, reader);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<BPackageDataReader> readerDeleter(reader);

	// check the offset
	if (offset < 0 || (uint64)offset > data.UncompressedSize())
		return B_BAD_VALUE;

	// clamp the size
	size_t toRead = std::min((uint64)*bufferSize,
		data.UncompressedSize() - offset);

	// read
	if (toRead > 0) {
		status_t error = reader->ReadData(offset, buffer, toRead);
		if (error != B_OK)
			RETURN_ERROR(error);
	}

	*bufferSize = toRead;
	return B_OK;
}


UnpackingAttributeCookie::UnpackingAttributeCookie(PackageNode* packageNode,
	PackageNodeAttribute* attribute, int openMode)
	:
	fPackageNode(packageNode),
	fPackage(packageNode->GetPackage()),
	fAttribute(attribute),
	fOpenMode(openMode)
{
	fPackageNode->AcquireReference();
	fPackage->AcquireReference();
}


UnpackingAttributeCookie::~UnpackingAttributeCookie()
{
	fPackageNode->ReleaseReference();
	fPackage->ReleaseReference();
}


/*static*/ status_t
UnpackingAttributeCookie::Open(PackageNode* packageNode, const char* name,
	int openMode, AttributeCookie*& _cookie)
{
	if (packageNode == NULL)
		return B_ENTRY_NOT_FOUND;

	// get the attribute
	PackageNodeAttribute* attribute = packageNode->FindAttribute(name);
	if (attribute == NULL)
		return B_ENTRY_NOT_FOUND;

	// allocate the cookie
	UnpackingAttributeCookie* cookie = new(std::nothrow)
		UnpackingAttributeCookie(packageNode, attribute, openMode);
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	_cookie = cookie;
	return B_OK;
}


status_t
UnpackingAttributeCookie::ReadAttribute(off_t offset, void* buffer,
	size_t* bufferSize)
{
	const BPackageData& data = fAttribute->Data();
	if (data.IsEncodedInline()) {
		// inline data
		BBufferDataReader dataReader(data.InlineData(), data.CompressedSize());
		return read_package_data(data, &dataReader, offset, buffer, bufferSize);
	}

	// data not inline -- open the package
	int fd = fPackage->Open();
	if (fd < 0)
		RETURN_ERROR(fd);
	PackageCloser packageCloser(fPackage);

	BFDDataReader dataReader(fd);
	return read_package_data(data, &dataReader, offset, buffer, bufferSize);
}


status_t
UnpackingAttributeCookie::ReadAttributeStat(struct stat* st)
{
	st->st_size = fAttribute->Data().UncompressedSize();
	st->st_type = fAttribute->Type();

	return B_OK;
}
