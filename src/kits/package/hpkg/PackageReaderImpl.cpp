/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PackageReaderImpl.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <new>

#include <ByteOrder.h>

#include <package/hpkg/HPKGDefsPrivate.h>

#include <package/hpkg/DataOutput.h>
#include <package/hpkg/ErrorOutput.h>
#include <package/hpkg/PackageData.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>
#include <package/hpkg/ZlibDecompressor.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


//#define TRACE(format...)	printf(format)
#define TRACE(format...)	do {} while (false)


// maximum TOC size we support reading
static const size_t kMaxTOCSize					= 64 * 1024 * 1024;

// maximum package attributes size we support reading
static const size_t kMaxPackageAttributesSize	= 1 * 1024 * 1024;


// #pragma mark - DataAttributeHandler


struct PackageReaderImpl::DataAttributeHandler : AttributeHandler {
	DataAttributeHandler(BPackageData* data)
		:
		fData(data)
	{
	}

	static status_t InitData(AttributeHandlerContext* context,
		BPackageData* data, const AttributeValue& value)
	{
		if (value.encoding == B_HPKG_ATTRIBUTE_ENCODING_RAW_INLINE)
			data->SetData(value.data.size, value.data.raw);
		else
			data->SetData(value.data.size, value.data.offset);

		data->SetUncompressedSize(value.data.size);

		return B_OK;
	}

	static status_t Create(AttributeHandlerContext* context,
		BPackageData* data, const AttributeValue& value,
		AttributeHandler*& _handler)
	{
		DataAttributeHandler* handler = new(std::nothrow) DataAttributeHandler(
			data);
		if (handler == NULL)
			return B_NO_MEMORY;

		InitData(context, data, value);

		_handler = handler;
		return B_OK;
	}

	virtual status_t HandleAttribute(AttributeHandlerContext* context,
		uint8 id, const AttributeValue& value, AttributeHandler** _handler)
	{
		switch (id) {
			case B_HPKG_ATTRIBUTE_ID_DATA_SIZE:
				fData->SetUncompressedSize(value.unsignedInt);
				return B_OK;

			case B_HPKG_ATTRIBUTE_ID_DATA_COMPRESSION:
			{
				switch (value.unsignedInt) {
					case B_HPKG_COMPRESSION_NONE:
					case B_HPKG_COMPRESSION_ZLIB:
						break;
					default:
						context->errorOutput->PrintError("Error: Invalid "
							"compression type for data (%llu)\n",
							value.unsignedInt);
						return B_BAD_DATA;
				}

				fData->SetCompression(value.unsignedInt);
				return B_OK;
			}

			case B_HPKG_ATTRIBUTE_ID_DATA_CHUNK_SIZE:
				fData->SetChunkSize(value.unsignedInt);
				return B_OK;
		}

		return AttributeHandler::HandleAttribute(context, id, value, _handler);
	}

private:
	BPackageData*	fData;
};


// #pragma mark - AttributeAttributeHandler


struct PackageReaderImpl::AttributeAttributeHandler : AttributeHandler {
	AttributeAttributeHandler(BPackageEntry* entry, const char* name)
		:
		fEntry(entry),
		fAttribute(name)
	{
	}

	virtual status_t HandleAttribute(AttributeHandlerContext* context,
		uint8 id, const AttributeValue& value, AttributeHandler** _handler)
	{
		switch (id) {
			case B_HPKG_ATTRIBUTE_ID_DATA:
				if (_handler != NULL) {
					return DataAttributeHandler::Create(context,
						&fAttribute.Data(), value, *_handler);
				}
				return DataAttributeHandler::InitData(context,
					&fAttribute.Data(), value);

			case B_HPKG_ATTRIBUTE_ID_FILE_ATTRIBUTE_TYPE:
				fAttribute.SetType(value.unsignedInt);
				return B_OK;
		}

		return AttributeHandler::HandleAttribute(context, id, value, _handler);
	}

	virtual status_t Delete(AttributeHandlerContext* context)
	{
		status_t error = context->packageContentHandler->HandleEntryAttribute(
			fEntry, &fAttribute);

		delete this;
		return error;
	}

private:
	BPackageEntry*			fEntry;
	BPackageEntryAttribute	fAttribute;
};


// #pragma mark - EntryAttributeHandler


struct PackageReaderImpl::EntryAttributeHandler : AttributeHandler {
	EntryAttributeHandler(AttributeHandlerContext* context,
		BPackageEntry* parentEntry, const char* name)
		:
		fEntry(parentEntry, name),
		fNotified(false)
	{
		_SetFileType(context, B_HPKG_DEFAULT_FILE_TYPE);
	}

	static status_t Create(AttributeHandlerContext* context,
		BPackageEntry* parentEntry, const char* name,
		AttributeHandler*& _handler)
	{
		// check name
		if (name[0] == '\0' || strcmp(name, ".") == 0
			|| strcmp(name, "..") == 0 || strchr(name, '/') != NULL) {
			context->errorOutput->PrintError("Error: Invalid package: Invalid "
				"entry name: \"%s\"\n", name);
			return B_BAD_DATA;
		}

		// create handler
		EntryAttributeHandler* handler = new(std::nothrow)
			EntryAttributeHandler(context, parentEntry, name);
		if (handler == NULL)
			return B_NO_MEMORY;

		_handler = handler;
		return B_OK;
	}

	virtual status_t HandleAttribute(AttributeHandlerContext* context,
		uint8 id, const AttributeValue& value, AttributeHandler** _handler)
	{
		switch (id) {
			case B_HPKG_ATTRIBUTE_ID_DIRECTORY_ENTRY:
			{
				status_t error = _Notify(context);
				if (error != B_OK)
					return error;

//TRACE("%*sentry \"%s\"\n", fLevel * 2, "", value.string);
				if (_handler != NULL) {
					return EntryAttributeHandler::Create(context, &fEntry,
						value.string, *_handler);
				}
				return B_OK;
			}

			case B_HPKG_ATTRIBUTE_ID_FILE_TYPE:
				return _SetFileType(context, value.unsignedInt);

			case B_HPKG_ATTRIBUTE_ID_FILE_PERMISSIONS:
				fEntry.SetPermissions(value.unsignedInt);
				return B_OK;

			case B_HPKG_ATTRIBUTE_ID_FILE_USER:
			case B_HPKG_ATTRIBUTE_ID_FILE_GROUP:
				// TODO:...
				break;

			case B_HPKG_ATTRIBUTE_ID_FILE_ATIME:
				fEntry.SetAccessTime(value.unsignedInt);
				return B_OK;

			case B_HPKG_ATTRIBUTE_ID_FILE_MTIME:
				fEntry.SetModifiedTime(value.unsignedInt);
				return B_OK;

			case B_HPKG_ATTRIBUTE_ID_FILE_CRTIME:
				fEntry.SetCreationTime(value.unsignedInt);
				return B_OK;

			case B_HPKG_ATTRIBUTE_ID_FILE_ATIME_NANOS:
				fEntry.SetAccessTimeNanos(value.unsignedInt);
				return B_OK;

			case B_HPKG_ATTRIBUTE_ID_FILE_MTIME_NANOS:
				fEntry.SetModifiedTimeNanos(value.unsignedInt);
				return B_OK;

			case B_HPKG_ATTRIBUTE_ID_FILE_CRTIM_NANOS:
				fEntry.SetCreationTimeNanos(value.unsignedInt);
				return B_OK;

			case B_HPKG_ATTRIBUTE_ID_FILE_ATTRIBUTE:
			{
				status_t error = _Notify(context);
				if (error != B_OK)
					return error;

				if (_handler != NULL) {
					*_handler = new(std::nothrow) AttributeAttributeHandler(
						&fEntry, value.string);
					if (*_handler == NULL)
						return B_NO_MEMORY;
					return B_OK;
				} else {
					BPackageEntryAttribute attribute(value.string);
					return context->packageContentHandler->HandleEntryAttribute(
						&fEntry, &attribute);
				}
			}

			case B_HPKG_ATTRIBUTE_ID_DATA:
				if (_handler != NULL) {
					return DataAttributeHandler::Create(context, &fEntry.Data(),
						value, *_handler);
				}
				return DataAttributeHandler::InitData(context, &fEntry.Data(),
					value);

			case B_HPKG_ATTRIBUTE_ID_SYMLINK_PATH:
				fEntry.SetSymlinkPath(value.string);
				return B_OK;
		}

		return AttributeHandler::HandleAttribute(context, id, value, _handler);
	}

	virtual status_t Delete(AttributeHandlerContext* context)
	{
		// notify if not done yet
		status_t error = _Notify(context);

		// notify done
		if (error == B_OK)
			error = context->packageContentHandler->HandleEntryDone(&fEntry);
		else
			context->packageContentHandler->HandleEntryDone(&fEntry);

		delete this;
		return error;
	}

private:
	status_t _Notify(AttributeHandlerContext* context)
	{
		if (fNotified)
			return B_OK;

		fNotified = true;
		return context->packageContentHandler->HandleEntry(&fEntry);
	}

	status_t _SetFileType(AttributeHandlerContext* context, uint64 fileType)
	{
		switch (fileType) {
			case B_HPKG_FILE_TYPE_FILE:
				fEntry.SetType(S_IFREG);
				fEntry.SetPermissions(B_HPKG_DEFAULT_FILE_PERMISSIONS);
				break;

			case B_HPKG_FILE_TYPE_DIRECTORY:
				fEntry.SetType(S_IFDIR);
				fEntry.SetPermissions(B_HPKG_DEFAULT_DIRECTORY_PERMISSIONS);
				break;

			case B_HPKG_FILE_TYPE_SYMLINK:
				fEntry.SetType(S_IFLNK);
				fEntry.SetPermissions(B_HPKG_DEFAULT_SYMLINK_PERMISSIONS);
				break;

			default:
				context->errorOutput->PrintError("Error: Invalid file type for "
					"package entry (%llu)\n", fileType);
				return B_BAD_DATA;
		}
		return B_OK;
	}

private:
	BPackageEntry	fEntry;
	bool			fNotified;
};


// #pragma mark - RootAttributeHandler


struct PackageReaderImpl::RootAttributeHandler : PackageAttributeHandler {
	typedef PackageAttributeHandler inherited;

	virtual status_t HandleAttribute(AttributeHandlerContext* context,
		uint8 id, const AttributeValue& value, AttributeHandler** _handler)
	{
		if (id == B_HPKG_ATTRIBUTE_ID_DIRECTORY_ENTRY) {
			if (_handler != NULL) {
				return EntryAttributeHandler::Create(context, NULL,
					value.string, *_handler);
			}
			return B_OK;
		}

		return inherited::HandleAttribute(context, id, value, _handler);
	}
};


// #pragma mark - PackageReaderImpl


PackageReaderImpl::PackageReaderImpl(BErrorOutput* errorOutput)
	:
	inherited(errorOutput),
	fTOCSection("TOC")
{
}


PackageReaderImpl::~PackageReaderImpl()
{
}


status_t
PackageReaderImpl::Init(const char* fileName)
{
	// open file
	int fd = open(fileName, O_RDONLY);
	if (fd < 0) {
		ErrorOutput()->PrintError("Error: Failed to open package file \"%s\": "
			"%s\n", fileName, strerror(errno));
		return errno;
	}

	return Init(fd, true);
}


status_t
PackageReaderImpl::Init(int fd, bool keepFD)
{
	status_t error = inherited::Init(fd, keepFD);
	if (error != B_OK)
		return error;

	// stat it
	struct stat st;
	if (fstat(FD(), &st) < 0) {
		ErrorOutput()->PrintError("Error: Failed to access package file: %s\n",
			strerror(errno));
		return errno;
	}

	// read the header
	hpkg_header header;
	if ((error = ReadBuffer(0, &header, sizeof(header))) != B_OK)
		return error;

	// check the header

	// magic
	if (B_BENDIAN_TO_HOST_INT32(header.magic) != B_HPKG_MAGIC) {
		ErrorOutput()->PrintError("Error: Invalid package file: Invalid "
			"magic\n");
		return B_BAD_DATA;
	}

	// header size
	fHeapOffset = B_BENDIAN_TO_HOST_INT16(header.header_size);
	if ((size_t)fHeapOffset < sizeof(hpkg_header)) {
		ErrorOutput()->PrintError("Error: Invalid package file: Invalid header "
			"size (%llu)\n", fHeapOffset);
		return B_BAD_DATA;
	}

	// version
	if (B_BENDIAN_TO_HOST_INT16(header.version) != B_HPKG_VERSION) {
		ErrorOutput()->PrintError("Error: Invalid/unsupported package file "
			"version (%d)\n", B_BENDIAN_TO_HOST_INT16(header.version));
		return B_BAD_DATA;
	}

	// total size
	fTotalSize = B_BENDIAN_TO_HOST_INT64(header.total_size);
	if (fTotalSize != (uint64)st.st_size) {
		ErrorOutput()->PrintError("Error: Invalid package file: Total size in "
			"header (%llu) doesn't agree with total file size (%lld)\n",
			fTotalSize, st.st_size);
		return B_BAD_DATA;
	}

	// package attributes length and compression
	fPackageAttributesSection.compression
		= B_BENDIAN_TO_HOST_INT32(header.attributes_compression);
	fPackageAttributesSection.compressedLength
		= B_BENDIAN_TO_HOST_INT32(header.attributes_length_compressed);
	fPackageAttributesSection.uncompressedLength
		= B_BENDIAN_TO_HOST_INT32(header.attributes_length_uncompressed);
	fPackageAttributesSection.stringsLength
		= B_BENDIAN_TO_HOST_INT32(header.attributes_strings_length);
	fPackageAttributesSection.stringsCount
		= B_BENDIAN_TO_HOST_INT32(header.attributes_strings_count);

	if (const char* errorString = CheckCompression(
		fPackageAttributesSection)) {
		ErrorOutput()->PrintError("Error: Invalid package file: package "
			"attributes section: %s\n", errorString);
		return B_BAD_DATA;
	}

	// TOC length and compression
	fTOCSection.compression = B_BENDIAN_TO_HOST_INT32(header.toc_compression);
	fTOCSection.compressedLength
		= B_BENDIAN_TO_HOST_INT64(header.toc_length_compressed);
	fTOCSection.uncompressedLength
		= B_BENDIAN_TO_HOST_INT64(header.toc_length_uncompressed);

	if (const char* errorString = CheckCompression(fTOCSection)) {
		ErrorOutput()->PrintError("Error: Invalid package file: TOC section: "
			"%s\n", errorString);
		return B_BAD_DATA;
	}

	// TOC subsections
	fTOCSection.stringsLength
		= B_BENDIAN_TO_HOST_INT64(header.toc_strings_length);
	fTOCSection.stringsCount
		= B_BENDIAN_TO_HOST_INT64(header.toc_strings_count);

	if (fTOCSection.stringsLength > fTOCSection.uncompressedLength
		|| fTOCSection.stringsCount > fTOCSection.stringsLength) {
		ErrorOutput()->PrintError("Error: Invalid package file: Invalid TOC "
			"subsections description\n");
		return B_BAD_DATA;
	}

	// check whether the sections fit together
	if (fPackageAttributesSection.compressedLength > fTotalSize
		|| fTOCSection.compressedLength
			> fTotalSize - fPackageAttributesSection.compressedLength
		|| fHeapOffset
			> fTotalSize - fPackageAttributesSection.compressedLength
				- fTOCSection.compressedLength) {
		ErrorOutput()->PrintError("Error: Invalid package file: The sum of the "
			"sections sizes is greater than the package size\n");
		return B_BAD_DATA;
	}

	fPackageAttributesSection.offset
		= fTotalSize - fPackageAttributesSection.compressedLength;
	fTOCSection.offset = fPackageAttributesSection.offset
		- fTOCSection.compressedLength;
	fHeapSize = fTOCSection.offset - fHeapOffset;

	// TOC size sanity check
	if (fTOCSection.uncompressedLength > kMaxTOCSize) {
		ErrorOutput()->PrintError("Error: Package file TOC section size "
			"is %llu bytes. This is beyond the reader's sanity limit\n",
			fTOCSection.uncompressedLength);
		return B_UNSUPPORTED;
	}

	// package attributes size sanity check
	if (fPackageAttributesSection.uncompressedLength
			> kMaxPackageAttributesSize) {
		ErrorOutput()->PrintError(
			"Error: Package file package attributes section size "
			"is %llu bytes. This is beyond the reader's sanity limit\n",
			fPackageAttributesSection.uncompressedLength);
		return B_UNSUPPORTED;
	}

	// read in the complete TOC
	fTOCSection.data
		= new(std::nothrow) uint8[fTOCSection.uncompressedLength];
	if (fTOCSection.data == NULL) {
		ErrorOutput()->PrintError("Error: Out of memory!\n");
		return B_NO_MEMORY;
	}
	error = ReadCompressedBuffer(fTOCSection);
	if (error != B_OK)
		return error;

	// read in the complete package attributes section
	fPackageAttributesSection.data
		= new(std::nothrow) uint8[fPackageAttributesSection.uncompressedLength];
	if (fPackageAttributesSection.data == NULL) {
		ErrorOutput()->PrintError("Error: Out of memory!\n");
		return B_NO_MEMORY;
	}
	error = ReadCompressedBuffer(fPackageAttributesSection);
	if (error != B_OK)
		return error;

	// start parsing the TOC
	fTOCSection.currentOffset = 0;
	SetCurrentSection(&fTOCSection);

	// strings
	error = ParseStrings();
	if (error != B_OK)
		return error;

	// parse strings from package attributes section
	fPackageAttributesSection.currentOffset = 0;
	SetCurrentSection(&fPackageAttributesSection);

	// strings
	error = ParseStrings();
	if (error != B_OK)
		return error;

	SetCurrentSection(NULL);

	return B_OK;
}


status_t
PackageReaderImpl::ParseContent(BPackageContentHandler* contentHandler)
{
	AttributeHandlerContext context(ErrorOutput(), contentHandler,
		B_HPKG_SECTION_PACKAGE_ATTRIBUTES);
	RootAttributeHandler rootAttributeHandler;

	status_t error
		= ParsePackageAttributesSection(&context, &rootAttributeHandler);

	if (error == B_OK) {
		context.section = B_HPKG_SECTION_PACKAGE_TOC;
		error = _ParseTOC(&context, &rootAttributeHandler);
	}

	return error;
}


status_t
PackageReaderImpl::ParseContent(BLowLevelPackageContentHandler* contentHandler)
{
	AttributeHandlerContext context(ErrorOutput(), contentHandler,
		B_HPKG_SECTION_PACKAGE_ATTRIBUTES);
	LowLevelAttributeHandler rootAttributeHandler;

	status_t error
		= ParsePackageAttributesSection(&context, &rootAttributeHandler);

	if (error == B_OK) {
		context.section = B_HPKG_SECTION_PACKAGE_TOC;
		error = _ParseTOC(&context, &rootAttributeHandler);
	}

	return error;
}


status_t
PackageReaderImpl::_ParseTOC(AttributeHandlerContext* context,
	AttributeHandler* rootAttributeHandler)
{
	// parse the TOC
	fTOCSection.currentOffset = fTOCSection.stringsLength;
	SetCurrentSection(&fTOCSection);

	// prepare attribute handler context
	context->heapOffset = fHeapOffset;
	context->heapSize = fHeapSize;

	// init the attribute handler stack
	rootAttributeHandler->SetLevel(0);
	ClearAttributeHandlerStack();
	PushAttributeHandler(rootAttributeHandler);

	bool sectionHandled;
	status_t error = ParseAttributeTree(context, sectionHandled);
	if (error == B_OK && sectionHandled) {
		if (fTOCSection.currentOffset < fTOCSection.uncompressedLength) {
			ErrorOutput()->PrintError("Error: %llu excess byte(s) in TOC "
				"section\n",
				fTOCSection.uncompressedLength - fTOCSection.currentOffset);
			error = B_BAD_DATA;
		}
	}

	// clean up on error
	if (error != B_OK) {
		context->ErrorOccurred();
		while (AttributeHandler* handler = PopAttributeHandler()) {
			if (handler != rootAttributeHandler)
				handler->Delete(context);
		}
		return error;
	}

	return B_OK;
}


status_t
PackageReaderImpl::ReadAttributeValue(uint8 type, uint8 encoding,
	AttributeValue& _value)
{
	switch (type) {
		case B_HPKG_ATTRIBUTE_TYPE_RAW:
		{
			uint64 size;
			status_t error = ReadUnsignedLEB128(size);
			if (error != B_OK)
				return error;

			if (encoding == B_HPKG_ATTRIBUTE_ENCODING_RAW_HEAP) {
				uint64 offset;
				error = ReadUnsignedLEB128(offset);
				if (error != B_OK)
					return error;

				if (offset > fHeapSize || size > fHeapSize - offset) {
					ErrorOutput()->PrintError("Error: Invalid %s section: "
						"invalid data reference\n", CurrentSection()->name);
					return B_BAD_DATA;
				}

				_value.SetToData(size, fHeapOffset + offset);
			} else if (encoding == B_HPKG_ATTRIBUTE_ENCODING_RAW_INLINE) {
				if (size > B_HPKG_MAX_INLINE_DATA_SIZE) {
					ErrorOutput()->PrintError("Error: Invalid %s section: "
						"inline data too long\n", CurrentSection()->name);
					return B_BAD_DATA;
				}

				const void* buffer;
				error = _GetTOCBuffer(size, buffer);
				if (error != B_OK)
					return error;
				_value.SetToData(size, buffer);
			} else {
				ErrorOutput()->PrintError("Error: Invalid %s section: invalid "
					"raw encoding (%u)\n", CurrentSection()->name, encoding);
				return B_BAD_DATA;
			}

			return B_OK;
		}

		default:
			return inherited::ReadAttributeValue(type, encoding, _value);
	}
}


status_t
PackageReaderImpl::_GetTOCBuffer(size_t size, const void*& _buffer)
{
	if (size > fTOCSection.uncompressedLength - fTOCSection.currentOffset) {
		ErrorOutput()->PrintError("_GetTOCBuffer(%lu): read beyond TOC end\n",
			size);
		return B_BAD_DATA;
	}

	_buffer = fTOCSection.data + fTOCSection.currentOffset;
	fTOCSection.currentOffset += size;
	return B_OK;
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
