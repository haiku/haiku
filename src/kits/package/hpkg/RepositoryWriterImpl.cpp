/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/RepositoryWriterImpl.h>

#include <algorithm>
#include <new>

#include <ByteOrder.h>

#include <package/hpkg/haiku_package.h>

#include <package/PackageInfo.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


RepositoryWriterImpl::RepositoryWriterImpl(BRepositoryWriterListener* listener)
	:
	WriterImplBase(listener),
	fListener(listener)
{
}


RepositoryWriterImpl::~RepositoryWriterImpl()
{
}


status_t
RepositoryWriterImpl::Init(const char* fileName)
{
	try {
		return _Init(fileName);
	} catch (status_t error) {
		return error;
	} catch (std::bad_alloc) {
		fListener->PrintError("Out of memory!\n");
		return B_NO_MEMORY;
	}
}


status_t
RepositoryWriterImpl::AddPackage(const BEntry& packageEntry)
{
	try {
		return _AddPackage(packageEntry);
	} catch (status_t error) {
		return error;
	} catch (std::bad_alloc) {
		fListener->PrintError("Out of memory!\n");
		return B_NO_MEMORY;
	}
}


status_t
RepositoryWriterImpl::Finish()
{
	try {
		return _Finish();
	} catch (status_t error) {
		return error;
	} catch (std::bad_alloc) {
		fListener->PrintError("Out of memory!\n");
		return B_NO_MEMORY;
	}
}


status_t
RepositoryWriterImpl::_Init(const char* fileName)
{
	return WriterImplBase::Init(fileName, "repository");
}


status_t
RepositoryWriterImpl::_Finish()
{
	hpkg_repo_header header;

	// write package attributes
	off_t totalSize = _WritePackageAttributes(header);

	fListener->OnRepositorySizeInfo(sizeof(header),
		B_BENDIAN_TO_HOST_INT32(header.attributes_length_compressed),
		totalSize);

	// prepare the header

	// general
	header.magic = B_HOST_TO_BENDIAN_INT32(B_HPKG_REPO_MAGIC);
	header.header_size = B_HOST_TO_BENDIAN_INT16((uint16)sizeof(header));
	header.version = B_HOST_TO_BENDIAN_INT16(B_HPKG_REPO_VERSION);
	header.total_size = B_HOST_TO_BENDIAN_INT64(totalSize);

	// write the header
	WriteBuffer(&header, sizeof(header), 0);

	SetFinished(true);
	return B_OK;
}


status_t
RepositoryWriterImpl::_AddPackage(const BEntry& packageEntry)
{
//	TODO!

	return B_OK;
}


off_t
RepositoryWriterImpl::_WritePackageAttributes(hpkg_repo_header& header)
{
	// write the package attributes (zlib writer on top of a file writer)
	off_t startOffset = sizeof(header);
	FDDataWriter realWriter(FD(), startOffset, fListener);
	ZlibDataWriter zlibWriter(&realWriter);
	SetDataWriter(&zlibWriter);
	zlibWriter.Init();

	// write cached strings and package attributes tree
	uint32 stringsLengthUncompressed;
	uint32 stringsCount = WritePackageAttributes(PackageAttributes(),
		stringsLengthUncompressed);

	zlibWriter.Finish();
	off_t endOffset = realWriter.Offset();
	SetDataWriter(NULL);

	fListener->OnPackageAttributesSizeInfo(stringsCount,
		zlibWriter.BytesWritten());

	// update the header
	header.attributes_compression
		= B_HOST_TO_BENDIAN_INT32(B_HPKG_COMPRESSION_ZLIB);
	header.attributes_length_compressed
		= B_HOST_TO_BENDIAN_INT32(endOffset - startOffset);
	header.attributes_length_uncompressed
		= B_HOST_TO_BENDIAN_INT32(zlibWriter.BytesWritten());
	header.attributes_strings_count = B_HOST_TO_BENDIAN_INT32(stringsCount);
	header.attributes_strings_length
		= B_HOST_TO_BENDIAN_INT32(stringsLengthUncompressed);

	return endOffset;
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
