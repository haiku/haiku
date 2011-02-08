/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/RepositoryWriterImpl.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <new>

#include <ByteOrder.h>
#include <Entry.h>
#include <fs_attr.h>

#include <AutoDeleter.h>

#include <package/hpkg/haiku_package.h>

#include <package/hpkg/DataOutput.h>
#include <package/hpkg/DataReader.h>
#include <package/hpkg/ZlibCompressor.h>


using BPrivate::FileDescriptorCloser;


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


RepositoryWriterImpl::RepositoryWriterImpl(BRepositoryWriterListener* listener)
	:
	fListener(listener),
	fFileName(NULL),
	fFD(-1),
	fFinished(false),
	fDataWriter(NULL),
	fCachedStrings(NULL)
{
}


RepositoryWriterImpl::~RepositoryWriterImpl()
{
	if (fCachedStrings != NULL) {
		CachedString* cachedString = fCachedStrings->Clear(true);
		while (cachedString != NULL) {
			CachedString* next = cachedString->next;
			delete cachedString;
			cachedString = next;
		}
	}

	if (fFD >= 0)
		close(fFD);

	if (!fFinished && fFileName != NULL)
		unlink(fFileName);
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
RepositoryWriterImpl::AddPackage(const BPackageInfo& packageInfo)
{
	try {
		return _RegisterPackage(packageInfo);
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
	// create hash tables
	fCachedStrings = new CachedStringTable;
	if (fCachedStrings->Init() != B_OK)
		throw std::bad_alloc();

	// allocate data buffer
	fDataBufferSize = 2 * B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB;
	fDataBuffer = malloc(fDataBufferSize);
	if (fDataBuffer == NULL)
		throw std::bad_alloc();

	// open file
	fFD = open(fileName, O_WRONLY | O_CREAT | O_TRUNC,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fFD < 0) {
		fListener->PrintError("Failed to open package file \"%s\": %s\n",
			fileName, strerror(errno));
		return errno;
	}

	fFileName = fileName;

	return B_OK;
}


status_t
RepositoryWriterImpl::_Finish()
{
	hpkg_repo_header header;

	// write the cached strings and package attributes
	_WriteCachedStrings();
	WritePackageAttributes(header);

	off_t totalSize = fHeapEnd;

	fListener->OnPackageSizeInfo(sizeof(hpkg_header), heapSize,
		B_BENDIAN_TO_HOST_INT64(header.toc_length_compressed),
		B_BENDIAN_TO_HOST_INT32(header.attributes_length_compressed),
		totalSize);

	// prepare the header

	// general
	header.magic = B_HOST_TO_BENDIAN_INT32(B_HPKG_REPO_MAGIC);
	header.header_size = B_HOST_TO_BENDIAN_INT16((uint16)sizeof(header));
	header.version = B_HOST_TO_BENDIAN_INT16(B_HPKG_REPO_VERSION);
	header.total_size = B_HOST_TO_BENDIAN_INT64(totalSize);

	// write the header
	_WriteBuffer(&header, sizeof(header), 0);

	fFinished = true;
	return B_OK;
}


status_t
RepositoryWriterImpl::_AddPackage(const BPackageInfo& packageInfo)
{
//	fPackageInfos.AddItem(packageInfo);

	return B_OK;
}


void
RepositoryWriterImpl::_WriteTOC(hpkg_header& header)
{
	// prepare the writer (zlib writer on top of a file writer)
	off_t startOffset = fHeapEnd;
	FDDataWriter realWriter(fFD, startOffset, fListener);
	ZlibDataWriter zlibWriter(&realWriter);
	fDataWriter = &zlibWriter;
	zlibWriter.Init();

	// write the sections
	uint64 uncompressedAttributeTypesSize;
	uint64 uncompressedStringsSize;
	uint64 uncompressedMainSize;
	int32 cachedStringsWritten = _WriteTOCSections(
		uncompressedAttributeTypesSize, uncompressedStringsSize,
		uncompressedMainSize);

	// finish the writer
	zlibWriter.Finish();
	fHeapEnd = realWriter.Offset();
	fDataWriter = NULL;

	off_t endOffset = fHeapEnd;

	fListener->OnTOCSizeInfo(uncompressedAttributeTypesSize,
		uncompressedStringsSize, uncompressedMainSize,
		zlibWriter.BytesWritten());

	// update the header

	// TOC
	header.toc_compression = B_HOST_TO_BENDIAN_INT32(B_HPKG_COMPRESSION_ZLIB);
	header.toc_length_compressed = B_HOST_TO_BENDIAN_INT64(
		endOffset - startOffset);
	header.toc_length_uncompressed = B_HOST_TO_BENDIAN_INT64(
		zlibWriter.BytesWritten());

	header.toc_strings_length = B_HOST_TO_BENDIAN_INT64(
		uncompressedStringsSize);
	header.toc_strings_count = B_HOST_TO_BENDIAN_INT64(cachedStringsWritten);
}


int32
RepositoryWriterImpl::_WriteTOCSections(uint64& _attributeTypesSize,
	uint64& _stringsSize, uint64& _mainSize)
{
	// write the attribute type abbreviations
	uint64 attributeTypesOffset = fDataWriter->BytesWritten();
	_WriteAttributeTypes();

	// write the cached strings
	uint64 cachedStringsOffset = fDataWriter->BytesWritten();
	int32 cachedStringsWritten = _WriteCachedStrings();

	// write the main TOC section
	uint64 mainOffset = fDataWriter->BytesWritten();
	_WriteAttributeChildren(fRootAttribute);

	_attributeTypesSize = cachedStringsOffset - attributeTypesOffset;
	_stringsSize = mainOffset - cachedStringsOffset;
	_mainSize = fDataWriter->BytesWritten() - mainOffset;

	return cachedStringsWritten;
}


void
RepositoryWriterImpl::_WritePackageAttributes(hpkg_header& header)
{
	// write the package attributes (zlib writer on top of a file writer)
	off_t startOffset = fHeapEnd;
	FDDataWriter realWriter(fFD, startOffset, fListener);
	ZlibDataWriter zlibWriter(&realWriter);
	fDataWriter = &zlibWriter;
	zlibWriter.Init();

	// name
	_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
		HPKG_PACKAGE_ATTRIBUTE_NAME, B_HPKG_ATTRIBUTE_TYPE_STRING,
		B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE, 0));
	_WriteString(fPackageInfo.Name().String());

	// summary
	_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
		HPKG_PACKAGE_ATTRIBUTE_SUMMARY, B_HPKG_ATTRIBUTE_TYPE_STRING,
		B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE, 0));
	_WriteString(fPackageInfo.Summary().String());

	// description
	_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
		HPKG_PACKAGE_ATTRIBUTE_DESCRIPTION, B_HPKG_ATTRIBUTE_TYPE_STRING,
		B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE, 0));
	_WriteString(fPackageInfo.Description().String());

	// vendor
	_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
		HPKG_PACKAGE_ATTRIBUTE_VENDOR, B_HPKG_ATTRIBUTE_TYPE_STRING,
		B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE, 0));
	_WriteString(fPackageInfo.Vendor().String());

	// packager
	_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
		HPKG_PACKAGE_ATTRIBUTE_PACKAGER, B_HPKG_ATTRIBUTE_TYPE_STRING,
		B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE, 0));
	_WriteString(fPackageInfo.Packager().String());

	// architecture
	_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
		HPKG_PACKAGE_ATTRIBUTE_ARCHITECTURE, B_HPKG_ATTRIBUTE_TYPE_UINT,
		B_HPKG_ATTRIBUTE_ENCODING_INT_8_BIT, 0));
	_Write<uint8>(fPackageInfo.Architecture());

	// version
	_WritePackageVersion(fPackageInfo.Version());

	// copyright list
	const BObjectList<BString>& copyrightList = fPackageInfo.CopyrightList();
	for (int i = 0; i < copyrightList.CountItems(); ++i) {
		_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
			HPKG_PACKAGE_ATTRIBUTE_COPYRIGHT, B_HPKG_ATTRIBUTE_TYPE_STRING,
			B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE, 0));
		_WriteString(copyrightList.ItemAt(i)->String());
	}

	// license list
	const BObjectList<BString>& licenseList = fPackageInfo.LicenseList();
	for (int i = 0; i < licenseList.CountItems(); ++i) {
		_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
			HPKG_PACKAGE_ATTRIBUTE_LICENSE, B_HPKG_ATTRIBUTE_TYPE_STRING,
			B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE, 0));
		_WriteString(licenseList.ItemAt(i)->String());
	}

	// provides list
	const BObjectList<BPackageResolvable>& providesList
		= fPackageInfo.ProvidesList();
	for (int i = 0; i < providesList.CountItems(); ++i) {
		BPackageResolvable* resolvable = providesList.ItemAt(i);
		bool hasVersion = resolvable->Version().InitCheck() == B_OK;
		_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
			HPKG_PACKAGE_ATTRIBUTE_PROVIDES_TYPE, B_HPKG_ATTRIBUTE_TYPE_UINT,
			B_HPKG_ATTRIBUTE_ENCODING_INT_8_BIT, 0));
		_Write<uint8>((uint8)resolvable->Type());
		_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
			HPKG_PACKAGE_ATTRIBUTE_PROVIDES, B_HPKG_ATTRIBUTE_TYPE_STRING,
			B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE, hasVersion));
		_WriteString(resolvable->Name().String());
		if (hasVersion) {
			_WritePackageVersion(resolvable->Version());
			_Write<uint8>(0);
		}
	}

	// requires list
	_WritePackageResolvableExpressionList(fPackageInfo.RequiresList(),
		HPKG_PACKAGE_ATTRIBUTE_REQUIRES);

	// supplements list
	_WritePackageResolvableExpressionList(fPackageInfo.SupplementsList(),
		HPKG_PACKAGE_ATTRIBUTE_SUPPLEMENTS);

	// conflicts list
	_WritePackageResolvableExpressionList(fPackageInfo.ConflictsList(),
		HPKG_PACKAGE_ATTRIBUTE_CONFLICTS);

	// freshens list
	_WritePackageResolvableExpressionList(fPackageInfo.FreshensList(),
		HPKG_PACKAGE_ATTRIBUTE_FRESHENS);

	// replaces list
	const BObjectList<BString>& replacesList = fPackageInfo.ReplacesList();
	for (int i = 0; i < replacesList.CountItems(); ++i) {
		_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
			HPKG_PACKAGE_ATTRIBUTE_REPLACES, B_HPKG_ATTRIBUTE_TYPE_STRING,
			B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE, 0));
		_WriteString(replacesList.ItemAt(i)->String());
	}

	_Write<uint8>(0);

	zlibWriter.Finish();
	fHeapEnd = realWriter.Offset();
	fDataWriter = NULL;

	off_t endOffset = fHeapEnd;

	fListener->OnPackageAttributesSizeInfo(zlibWriter.BytesWritten());

	// update the header
	header.attributes_compression
		= B_HOST_TO_BENDIAN_INT32(B_HPKG_COMPRESSION_ZLIB);
	header.attributes_length_compressed
		= B_HOST_TO_BENDIAN_INT32(endOffset - startOffset);
	header.attributes_length_uncompressed
		= B_HOST_TO_BENDIAN_INT32(zlibWriter.BytesWritten());
}


void
RepositoryWriterImpl::_WritePackageVersion(const BPackageVersion& version)
{
	_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
		HPKG_PACKAGE_ATTRIBUTE_VERSION_MAJOR, B_HPKG_ATTRIBUTE_TYPE_STRING,
		B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE, 0));
	_WriteString(version.Major());
	_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
		HPKG_PACKAGE_ATTRIBUTE_VERSION_MINOR, B_HPKG_ATTRIBUTE_TYPE_STRING,
		B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE, 0));
	_WriteString(version.Minor());
	_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
		HPKG_PACKAGE_ATTRIBUTE_VERSION_MICRO, B_HPKG_ATTRIBUTE_TYPE_STRING,
		B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE, 0));
	_WriteString(version.Micro());
	_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
		HPKG_PACKAGE_ATTRIBUTE_VERSION_RELEASE, B_HPKG_ATTRIBUTE_TYPE_UINT,
		B_HPKG_ATTRIBUTE_ENCODING_INT_8_BIT, 0));
	_Write<uint8>(version.Release());
}


void
RepositoryWriterImpl::_WritePackageResolvableExpressionList(
	const BObjectList<BPackageResolvableExpression>& list, uint8 id)
{
	for (int i = 0; i < list.CountItems(); ++i) {
		BPackageResolvableExpression* resolvableExpr = list.ItemAt(i);
		bool hasVersion = resolvableExpr->Version().InitCheck() == B_OK;
		_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
			id, B_HPKG_ATTRIBUTE_TYPE_STRING,
			B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE, hasVersion));
		_WriteString(resolvableExpr->Name().String());
		if (hasVersion) {
			_WriteUnsignedLEB128(HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(
				HPKG_PACKAGE_ATTRIBUTE_RESOLVABLE_OPERATOR,
				B_HPKG_ATTRIBUTE_TYPE_UINT, B_HPKG_ATTRIBUTE_ENCODING_INT_8_BIT,
				0));
			_Write<uint8>(resolvableExpr->Operator());
			_WritePackageVersion(resolvableExpr->Version());
			_Write<uint8>(0);
		}
	}
}


void
RepositoryWriterImpl::_WriteUnsignedLEB128(uint64 value)
{
	uint8 bytes[10];
	int32 count = 0;
	do {
		uint8 byte = value & 0x7f;
		value >>= 7;
		bytes[count++] = byte | (value != 0 ? 0x80 : 0);
	} while (value != 0);

	fDataWriter->WriteDataThrows(bytes, count);
}


void
RepositoryWriterImpl::_WriteBuffer(const void* buffer, size_t size, off_t offset)
{
	ssize_t bytesWritten = pwrite(fFD, buffer, size, offset);
	if (bytesWritten < 0) {
		fListener->PrintError(
			"_WriteBuffer(%p, %lu) failed to write data: %s\n", buffer, size,
			strerror(errno));
		throw status_t(errno);
	}
	if ((size_t)bytesWritten != size) {
		fListener->PrintError(
			"_WriteBuffer(%p, %lu) failed to write all data\n", buffer, size);
		throw status_t(B_ERROR);
	}
}


CachedString*
RepositoryWriterImpl::_GetCachedString(const char* value)
{
	CachedString* string = fCachedStrings->Lookup(value);
	if (string != NULL) {
		string->usageCount++;
		return string;
	}

	string = new CachedString;
	if (!string->Init(value)) {
		delete string;
		throw std::bad_alloc();
	}

	fCachedStrings->Insert(string);
	return string;
}


status_t
RepositoryWriterImpl::_WriteZlibCompressedData(BDataReader& dataReader, off_t size,
	uint64 writeOffset, uint64& _compressedSize)
{
	// Use zlib compression only for data large enough.
	if (size < kZlibCompressionSizeThreshold)
		return B_BAD_VALUE;

	// fDataBuffer is 2 * B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB, so split it into
	// two halves we can use for reading and compressing
	const size_t chunkSize = B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB;
	uint8* inputBuffer = (uint8*)fDataBuffer;
	uint8* outputBuffer = (uint8*)fDataBuffer + chunkSize;

	// account for the offset table
	uint64 chunkCount = (size + (chunkSize - 1)) / chunkSize;
	off_t offsetTableOffset = writeOffset;
	uint64* offsetTable = NULL;
	if (chunkCount > 1) {
		offsetTable = new uint64[chunkCount - 1];
		writeOffset = offsetTableOffset + (chunkCount - 1) * sizeof(uint64);
	}
	ArrayDeleter<uint64> offsetTableDeleter(offsetTable);

	const uint64 dataOffset = writeOffset;
	const uint64 dataEndLimit = offsetTableOffset + size;

	// read the data, compress them and write them to the heap
	off_t readOffset = 0;
	off_t remainingSize = size;
	uint64 chunkIndex = 0;
	while (remainingSize > 0) {
		// read data
		size_t toCopy = std::min(remainingSize, (off_t)chunkSize);
		status_t error = dataReader.ReadData(readOffset, inputBuffer, toCopy);
		if (error != B_OK) {
			fListener->PrintError("Failed to read data: %s\n", strerror(error));
			return error;
		}

		// compress
		size_t compressedSize;
		error = ZlibCompressor::CompressSingleBuffer(inputBuffer, toCopy,
			outputBuffer, toCopy, compressedSize);

		const void* writeBuffer;
		size_t bytesToWrite;
		if (error == B_OK) {
			writeBuffer = outputBuffer;
			bytesToWrite = compressedSize;
		} else {
			if (error != B_BUFFER_OVERFLOW)
				return error;
			writeBuffer = inputBuffer;
			bytesToWrite = toCopy;
		}

		// check the total compressed data size
		if (writeOffset + bytesToWrite >= dataEndLimit)
			return B_BUFFER_OVERFLOW;

		if (chunkIndex > 0)
			offsetTable[chunkIndex - 1] = writeOffset - dataOffset;

		// write to heap
		ssize_t bytesWritten = pwrite(fFD, writeBuffer, bytesToWrite,
			writeOffset);
		if (bytesWritten < 0) {
			fListener->PrintError("Failed to write data: %s\n",
				strerror(errno));
			return errno;
		}
		if ((size_t)bytesWritten != bytesToWrite) {
			fListener->PrintError("Failed to write all data\n");
			return B_ERROR;
		}

		remainingSize -= toCopy;
		readOffset += toCopy;
		writeOffset += bytesToWrite;
		chunkIndex++;
	}

	// write the offset table
	if (chunkCount > 1) {
		size_t bytesToWrite = (chunkCount - 1) * sizeof(uint64);
		ssize_t bytesWritten = pwrite(fFD, offsetTable, bytesToWrite,
			offsetTableOffset);
		if (bytesWritten < 0) {
			fListener->PrintError("Failed to write data: %s\n",
				strerror(errno));
			return errno;
		}
		if ((size_t)bytesWritten != bytesToWrite) {
			fListener->PrintError("Failed to write all data\n");
			return B_ERROR;
		}
	}

	_compressedSize = writeOffset - offsetTableOffset;
	return B_OK;
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
