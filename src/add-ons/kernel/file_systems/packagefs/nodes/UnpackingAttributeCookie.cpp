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

#include "AttributeIndexer.h"
#include "AutoPackageAttributes.h"
#include "DebugSupport.h"
#include "Package.h"
#include "PackageNode.h"


using BPackageKit::BHPKG::BDataReader;
using BPackageKit::BHPKG::BBufferDataReader;
using BPackageKit::BHPKG::BFDDataReader;


static status_t
read_package_data(const PackageData& data, BDataReader* reader, off_t offset,
	void* buffer, size_t* bufferSize)
{
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
UnpackingAttributeCookie::Open(PackageNode* packageNode, const StringKey& name,
	int openMode, AttributeCookie*& _cookie)
{
	if (packageNode == NULL)
		return B_ENTRY_NOT_FOUND;

	// get the attribute
	PackageNodeAttribute* attribute = packageNode->FindAttribute(name);
	if (attribute == NULL) {
		// We don't know the attribute -- maybe it's an auto-generated one.
		return AutoPackageAttributes::OpenCookie(packageNode->GetPackage(),
			name, openMode, _cookie);
	}

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
	return ReadAttribute(fPackageNode, fAttribute, offset, buffer, bufferSize);
}


status_t
UnpackingAttributeCookie::ReadAttributeStat(struct stat* st)
{
	st->st_size = fAttribute->Data().UncompressedSize();
	st->st_type = fAttribute->Type();

	return B_OK;
}


/*static*/ status_t
UnpackingAttributeCookie::ReadAttribute(PackageNode* packageNode,
	PackageNodeAttribute* attribute, off_t offset, void* buffer,
	size_t* bufferSize)
{
	const PackageData& data = attribute->Data();
	if (data.IsEncodedInline()) {
		// inline data
		BBufferDataReader dataReader(data.InlineData(),
			data.UncompressedSize());
		return read_package_data(data, &dataReader, offset, buffer, bufferSize);
	}

	// data not inline -- open the package and let it create a data reader for
	// us
	Package* package = packageNode->GetPackage();
	int fd = package->Open();
	if (fd < 0)
		RETURN_ERROR(fd);
	PackageCloser packageCloser(package);

	BAbstractBufferedDataReader* reader;
	status_t error = package->CreateDataReader(data, reader);
	if (error != B_OK)
		return error;
	ObjectDeleter<BAbstractBufferedDataReader> readerDeleter(reader);

	return read_package_data(data, reader, offset, buffer, bufferSize);
}


/*static*/ status_t
UnpackingAttributeCookie::IndexAttribute(PackageNode* packageNode,
	AttributeIndexer* indexer)
{
	if (packageNode == NULL)
		return B_ENTRY_NOT_FOUND;

	// get the attribute
	PackageNodeAttribute* attribute = packageNode->FindAttribute(
		indexer->IndexName());
	if (attribute == NULL)
		return B_ENTRY_NOT_FOUND;

	// create the index cookie
	void* data;
	size_t toRead;
	status_t error = indexer->CreateCookie(packageNode, attribute,
		attribute->Type(), attribute->Data().UncompressedSize(), data, toRead);
	if (error != B_OK)
		return error;

	// read the attribute
	if (toRead > 0) {
		// The package must be open or otherwise reading the attribute data
		// may fail.
		int fd = packageNode->GetPackage()->Open();
		if (fd < 0) {
			indexer->DeleteCookie();
			return fd;
		}
		PackageCloser packageCloser(packageNode->GetPackage());

		error = ReadAttribute(packageNode, attribute, 0, data, &toRead);
		if (error != B_OK) {
			indexer->DeleteCookie();
			return error;
		}
	}

	attribute->SetIndexCookie(indexer->Cookie());

	return B_OK;
}
