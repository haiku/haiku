/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageReader.h"

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

#include <haiku_package.h>

#include "DataOutput.h"
#include "PackageData.h"
#include "PackageEntry.h"
#include "PackageEntryAttribute.h"
#include "ZlibDecompressor.h"


// maximum TOC size we support reading
static const size_t kMaxTOCSize			= 64 * 1024 * 1024;

static const size_t kScratchBufferSize	= 64 * 1024;


enum {
	ATTRIBUTE_INDEX_DIRECTORY_ENTRY = 0,
	ATTRIBUTE_INDEX_FILE_TYPE,
	ATTRIBUTE_INDEX_FILE_PERMISSIONS,
	ATTRIBUTE_INDEX_FILE_USER,
	ATTRIBUTE_INDEX_FILE_GROUP,
	ATTRIBUTE_INDEX_FILE_ATIME,
	ATTRIBUTE_INDEX_FILE_MTIME,
	ATTRIBUTE_INDEX_FILE_CRTIME,
	ATTRIBUTE_INDEX_FILE_ATIME_NANOS,
	ATTRIBUTE_INDEX_FILE_MTIME_NANOS,
	ATTRIBUTE_INDEX_FILE_CRTIM_NANOS,
	ATTRIBUTE_INDEX_FILE_ATTRIBUTE,
	ATTRIBUTE_INDEX_FILE_ATTRIBUTE_TYPE,
	ATTRIBUTE_INDEX_DATA,
	ATTRIBUTE_INDEX_DATA_SIZE,
	ATTRIBUTE_INDEX_DATA_COMPRESSION,
	ATTRIBUTE_INDEX_DATA_CHUNK_SIZE,
	ATTRIBUTE_INDEX_SYMLINK_PATH
};


struct standard_attribute_index_entry {
	const char*	name;
	uint8		type;
	int8		index;
};

#undef MAKE_ATTRIBUTE_INDEX_ENTRY
#define MAKE_ATTRIBUTE_INDEX_ENTRY(name, type)	\
	{ B_HPKG_ATTRIBUTE_NAME_##name, B_HPKG_ATTRIBUTE_TYPE_##type, \
		ATTRIBUTE_INDEX_##name }

static const standard_attribute_index_entry kStandardAttributeIndices[] = {
	MAKE_ATTRIBUTE_INDEX_ENTRY(DIRECTORY_ENTRY, STRING),
	MAKE_ATTRIBUTE_INDEX_ENTRY(FILE_TYPE, UINT),
	MAKE_ATTRIBUTE_INDEX_ENTRY(FILE_PERMISSIONS, UINT),
	MAKE_ATTRIBUTE_INDEX_ENTRY(FILE_USER, STRING),
	MAKE_ATTRIBUTE_INDEX_ENTRY(FILE_GROUP, STRING),
	MAKE_ATTRIBUTE_INDEX_ENTRY(FILE_ATIME, UINT),
	MAKE_ATTRIBUTE_INDEX_ENTRY(FILE_MTIME, UINT),
	MAKE_ATTRIBUTE_INDEX_ENTRY(FILE_CRTIME, UINT),
	MAKE_ATTRIBUTE_INDEX_ENTRY(FILE_ATIME_NANOS, UINT),
	MAKE_ATTRIBUTE_INDEX_ENTRY(FILE_MTIME_NANOS, UINT),
	MAKE_ATTRIBUTE_INDEX_ENTRY(FILE_CRTIM_NANOS, UINT),
	MAKE_ATTRIBUTE_INDEX_ENTRY(FILE_ATTRIBUTE, STRING),
	MAKE_ATTRIBUTE_INDEX_ENTRY(FILE_ATTRIBUTE_TYPE, UINT),
	MAKE_ATTRIBUTE_INDEX_ENTRY(DATA, RAW),
	MAKE_ATTRIBUTE_INDEX_ENTRY(DATA_SIZE, UINT),
	MAKE_ATTRIBUTE_INDEX_ENTRY(DATA_COMPRESSION, UINT),
	MAKE_ATTRIBUTE_INDEX_ENTRY(DATA_CHUNK_SIZE, UINT),
	MAKE_ATTRIBUTE_INDEX_ENTRY(SYMLINK_PATH, STRING),
	{}
};


// #pragma mark - LowLevelPackageContentHandler


LowLevelPackageContentHandler::~LowLevelPackageContentHandler()
{
}


// #pragma mark - PackageContentHandler


PackageContentHandler::~PackageContentHandler()
{
}


// #pragma mark - AttributeType


struct PackageReader::AttributeType {
	uint8	type;
	char	name[0];
};


// #pragma mark - AttributeType


struct PackageReader::AttributeTypeReference {
	AttributeType*	type;
	int32			standardIndex;
};


// #pragma mark - AttributeHandler


struct PackageReader::AttributeHandlerContext {
	union {
		PackageContentHandler*			packageContentHandler;
		LowLevelPackageContentHandler*	lowLevelPackageContentHandler;
	};
	bool					hasLowLevelHandler;

	uint64					heapOffset;
	uint64					heapSize;


	AttributeHandlerContext(PackageContentHandler* packageContentHandler)
		:
		packageContentHandler(packageContentHandler),
		hasLowLevelHandler(false)
	{
	}

	AttributeHandlerContext(
		LowLevelPackageContentHandler* lowLevelPackageContentHandler)
		:
		lowLevelPackageContentHandler(lowLevelPackageContentHandler),
		hasLowLevelHandler(true)
	{
	}

	void ErrorOccurred()
	{
		if (hasLowLevelHandler)
			lowLevelPackageContentHandler->HandleErrorOccurred();
		else
			packageContentHandler->HandleErrorOccurred();
	}
};


struct PackageReader::AttributeHandler
	: SinglyLinkedListLinkImpl<AttributeHandler> {

	virtual ~AttributeHandler()
	{
	}

	void SetLevel(int level)
	{
		fLevel = level;
	}

	virtual status_t HandleChildAttribute(AttributeHandlerContext* context,
		AttributeType* type, int8 typeIndex, const AttributeValue& value,
		AttributeHandler** _handler)
	{
printf("%*signored attribute \"%s\" (%u)\n", fLevel * 2, "", type->name, type->type);
		return B_OK;
	}

	virtual status_t Delete(AttributeHandlerContext* context)
	{
		delete this;
		return B_OK;
	}

protected:
	int	fLevel;
};


struct PackageReader::IgnoreAttributeHandler : AttributeHandler {
};


struct PackageReader::DataAttributeHandler : AttributeHandler {
	DataAttributeHandler(PackageData* data)
		:
		fData(data)
	{
	}

	static status_t InitData(AttributeHandlerContext* context,
		PackageData* data, const AttributeValue& value)
	{
		if (value.encoding == B_HPKG_ATTRIBUTE_ENCODING_RAW_INLINE)
			data->SetData(value.data.size, value.data.raw);
		else
			data->SetData(value.data.size, value.data.offset);

		data->SetUncompressedSize(value.data.size);

		return B_OK;
	}

	static status_t Create(AttributeHandlerContext* context,
		PackageData* data, const AttributeValue& value,
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

	virtual status_t HandleChildAttribute(AttributeHandlerContext* context,
		AttributeType* type, int8 typeIndex, const AttributeValue& value,
		AttributeHandler** _handler)
	{
		switch (typeIndex) {
			case ATTRIBUTE_INDEX_DATA_SIZE:
				fData->SetUncompressedSize(value.unsignedInt);
				return B_OK;

			case ATTRIBUTE_INDEX_DATA_COMPRESSION:
			{
				switch (value.unsignedInt) {
					case B_HPKG_COMPRESSION_NONE:
					case B_HPKG_COMPRESSION_ZLIB:
						break;
					default:
						fprintf(stderr, "Error: Invalid compression type for "
							"data (%llu)\n", value.unsignedInt);
						return B_BAD_DATA;
				}

				fData->SetCompression(value.unsignedInt);
				return B_OK;
			}

			case ATTRIBUTE_INDEX_DATA_CHUNK_SIZE:
				fData->SetChunkSize(value.unsignedInt);
				return B_OK;
		}

		return AttributeHandler::HandleChildAttribute(context, type, typeIndex,
			value, _handler);
	}

private:
	PackageData*	fData;
};


struct PackageReader::AttributeAttributeHandler : AttributeHandler {
	AttributeAttributeHandler(PackageEntry* entry, const char* name)
		:
		fEntry(entry),
		fAttribute(name)
	{
	}

	virtual status_t HandleChildAttribute(AttributeHandlerContext* context,
		AttributeType* type, int8 typeIndex, const AttributeValue& value,
		AttributeHandler** _handler)
	{
		switch (typeIndex) {
			case ATTRIBUTE_INDEX_DATA:
			{
				if (_handler != NULL) {
					return DataAttributeHandler::Create(context,
						&fAttribute.Data(), value, *_handler);
				}
				return DataAttributeHandler::InitData(context, &fAttribute.Data(),
					value);
			}

			case ATTRIBUTE_INDEX_FILE_ATTRIBUTE_TYPE:
				fAttribute.SetType(value.unsignedInt);
				return B_OK;
		}

		return AttributeHandler::HandleChildAttribute(context, type, typeIndex,
			value, _handler);
	}

	virtual status_t Delete(AttributeHandlerContext* context)
	{
		status_t error = context->packageContentHandler->HandleEntryAttribute(
			fEntry, &fAttribute);

		delete this;
		return error;
	}

private:
	PackageEntry*			fEntry;
	PackageEntryAttribute	fAttribute;
};


struct PackageReader::EntryAttributeHandler : AttributeHandler {
	EntryAttributeHandler(PackageEntry* parentEntry, const char* name)
		:
		fEntry(parentEntry, name),
		fNotified(false)
	{
		_SetFileType(B_HPKG_DEFAULT_FILE_TYPE);
	}

	static status_t Create(PackageEntry* parentEntry, const char* name,
		AttributeHandler*& _handler)
	{
		// check name
		if (name[0] == '\0' || strcmp(name, ".") == 0
			|| strcmp(name, "..") == 0 || strchr(name, '/') != NULL) {
			fprintf(stderr, "Error: Invalid package: Invalid entry name: "
				"\"%s\"\n", name);
			return B_BAD_DATA;
		}

		// create handler
		EntryAttributeHandler* handler = new(std::nothrow)
			EntryAttributeHandler(parentEntry, name);
		if (handler == NULL)
			return B_NO_MEMORY;

		_handler = handler;
		return B_OK;
	}

	virtual status_t HandleChildAttribute(AttributeHandlerContext* context,
		AttributeType* type, int8 typeIndex, const AttributeValue& value,
		AttributeHandler** _handler)
	{
		switch (typeIndex) {
			case ATTRIBUTE_INDEX_DIRECTORY_ENTRY:
			{
				status_t error = _Notify(context);
				if (error != B_OK)
					return error;

//printf("%*sentry \"%s\"\n", fLevel * 2, "", value.string);
				if (_handler != NULL) {
					return EntryAttributeHandler::Create(&fEntry, value.string,
						*_handler);
				}
				return B_OK;
			}

			case ATTRIBUTE_INDEX_FILE_TYPE:
				return _SetFileType(value.unsignedInt);

			case ATTRIBUTE_INDEX_FILE_PERMISSIONS:
				fEntry.SetPermissions(value.unsignedInt);
				return B_OK;

			case ATTRIBUTE_INDEX_FILE_USER:
			case ATTRIBUTE_INDEX_FILE_GROUP:
				// TODO:...
				break;

			case ATTRIBUTE_INDEX_FILE_ATIME:
				fEntry.SetAccessTime(value.unsignedInt);
				return B_OK;

			case ATTRIBUTE_INDEX_FILE_MTIME:
				fEntry.SetModifiedTime(value.unsignedInt);
				return B_OK;

			case ATTRIBUTE_INDEX_FILE_CRTIME:
				fEntry.SetCreationTime(value.unsignedInt);
				return B_OK;

			case ATTRIBUTE_INDEX_FILE_ATIME_NANOS:
				fEntry.SetAccessTimeNanos(value.unsignedInt);
				return B_OK;

			case ATTRIBUTE_INDEX_FILE_MTIME_NANOS:
				fEntry.SetModifiedTimeNanos(value.unsignedInt);
				return B_OK;

			case ATTRIBUTE_INDEX_FILE_CRTIM_NANOS:
				fEntry.SetCreationTimeNanos(value.unsignedInt);
				return B_OK;

			case ATTRIBUTE_INDEX_FILE_ATTRIBUTE:
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
					PackageEntryAttribute attribute(value.string);
					return context->packageContentHandler->HandleEntryAttribute(
						&fEntry, &attribute);
				}
			}

			case ATTRIBUTE_INDEX_DATA:
			{
				if (_handler != NULL) {
					return DataAttributeHandler::Create(context, &fEntry.Data(),
						value, *_handler);
				}
				return DataAttributeHandler::InitData(context, &fEntry.Data(),
					value);
			}

			case ATTRIBUTE_INDEX_SYMLINK_PATH:
			{
				fEntry.SetSymlinkPath(value.string);
				return B_OK;
			}
		}

		return AttributeHandler::HandleChildAttribute(context, type, typeIndex,
			value, _handler);
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

	status_t _SetFileType(uint64 fileType)
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
				fprintf(stderr, "Error: Invalid file type for package "
					"entry (%llu)\n", fileType);
				return B_BAD_DATA;
		}
		return B_OK;
	}

private:
	PackageEntry		fEntry;
	bool				fNotified;
};


struct PackageReader::RootAttributeHandler : AttributeHandler {
	virtual status_t HandleChildAttribute(AttributeHandlerContext* context,
		AttributeType* type, int8 typeIndex, const AttributeValue& value,
		AttributeHandler** _handler)
	{
		if (typeIndex == ATTRIBUTE_INDEX_DIRECTORY_ENTRY) {
//printf("%*sentry \"%s\"\n", fLevel * 2, "", value.string);
			if (_handler != NULL) {
				return EntryAttributeHandler::Create(NULL, value.string,
					*_handler);
			}
			return B_OK;
		}

		return AttributeHandler::HandleChildAttribute(context, type, typeIndex,
			value, _handler);
	}
};


struct PackageReader::PackageAttributeHandler : AttributeHandler {
	PackageAttributeHandler()
		:
		fToken(NULL),
		fAttributeName(NULL)
	{
	}

	PackageAttributeHandler(const char* attributeName,
		const PackageAttributeValue& value, void* token)
		:
		fToken(token),
		fAttributeName(attributeName),
		fValue(value)
	{
	}

	virtual status_t HandleChildAttribute(AttributeHandlerContext* context,
		AttributeType* type, int8 typeIndex, const AttributeValue& value,
		AttributeHandler** _handler)
	{
		// notify the content handler
		void* token;
		status_t error = context->lowLevelPackageContentHandler
			->HandleAttribute(type->name, value, fToken, token);
		if (error != B_OK)
			return error;

		// create a subhandler for the attribute, if it has children
		if (_handler != NULL) {
			*_handler = new(std::nothrow) PackageAttributeHandler(type->name,
				value, token);
			if (*_handler == NULL) {
				context->lowLevelPackageContentHandler->HandleAttributeDone(
					type->name, value, token);
				return B_NO_MEMORY;
			}
			return B_OK;
		}

		// no children -- just call the done hook
		return context->lowLevelPackageContentHandler->HandleAttributeDone(
			type->name, value, token);
	}

	virtual status_t Delete(AttributeHandlerContext* context)
	{
		if (fAttributeName != NULL) {
			return context->lowLevelPackageContentHandler->HandleAttributeDone(
				fAttributeName, fValue, fToken);
		}

		return B_OK;
	}

private:
	void*			fToken;
	const char*		fAttributeName;
	AttributeValue	fValue;
};


// #pragma mark - PackageReader


inline PackageReader::AttributeHandler*
PackageReader::_CurrentAttributeHandler() const
{
	return fAttributeHandlerStack->Head();
}


inline void
PackageReader::_PushAttributeHandler(AttributeHandler* handler)
{
	fAttributeHandlerStack->Add(handler);
}


inline PackageReader::AttributeHandler*
PackageReader::_PopAttributeHandler()
{
	return fAttributeHandlerStack->RemoveHead();
}


PackageReader::PackageReader()
	:
	fFD(-1),
	fTOCSection(NULL),
	fAttributeTypes(NULL),
	fStrings(NULL),
	fScratchBuffer(NULL),
	fScratchBufferSize(0)
{
}


PackageReader::~PackageReader()
{
	if (fFD >= 0)
		close(fFD);

	delete[] fScratchBuffer;
	delete[] fStrings;
	delete[] fAttributeTypes;
	delete[] fTOCSection;
}


status_t
PackageReader::Init(const char* fileName)
{
	// open file
	fFD = open(fileName, O_RDONLY);
	if (fFD < 0) {
		fprintf(stderr, "Error: Failed to open package file \"%s\": %s\n",
			fileName, strerror(errno));
		return errno;
	}

	// stat it
	struct stat st;
	if (fstat(fFD, &st) < 0) {
		fprintf(stderr, "Error: Failed to access package file \"%s\": %s\n",
			fileName, strerror(errno));
		return errno;
	}

	// read the header
	hpkg_header header;
	status_t error = _ReadBuffer(0, &header, sizeof(header));
	if (error != B_OK)
		return error;

	// check the header

	// magic
	if (B_BENDIAN_TO_HOST_INT32(header.magic) != B_HPKG_MAGIC) {
		fprintf(stderr, "Error: File \"%s\" is not a valid package file: "
			"Invalid magic\n", fileName);
		return B_BAD_DATA;
	}

	// header size
	fHeapOffset = B_BENDIAN_TO_HOST_INT16(header.header_size);
	if ((size_t)fHeapOffset < sizeof(hpkg_header)) {
		fprintf(stderr, "Error: File \"%s\" is not a valid package file: "
			"Invalid header size (%llu)\n", fileName, fHeapOffset);
		return B_BAD_DATA;
	}

	// version
	if (B_BENDIAN_TO_HOST_INT16(header.version) != B_HPKG_VERSION) {
		fprintf(stderr, "Error: Can't read package file \"%s\": "
			"Invalid/unsupported version (%d)\n", fileName,
			B_BENDIAN_TO_HOST_INT16(header.version));
		return B_BAD_DATA;
	}

	// total size
	fTotalSize = B_BENDIAN_TO_HOST_INT64(header.total_size);
	if (fTotalSize != (uint64)st.st_size) {
		fprintf(stderr, "Error: File \"%s\" is not a valid package file: "
			"Total size in header (%llu) doesn't agree with total file size "
			"(%lld)\n", fileName, fTotalSize, st.st_size);
		return B_BAD_DATA;
	}

	// package attributes length and compression
	fPackageAttributesCompression
		= B_BENDIAN_TO_HOST_INT32(header.attributes_compression);
	fPackageAttributesCompressedLength
		= B_BENDIAN_TO_HOST_INT32(header.attributes_length_compressed);
	fPackageAttributesUncompressedLength
		= B_BENDIAN_TO_HOST_INT32(header.attributes_length_uncompressed);

	if (const char* errorString = _CheckCompression(
			fPackageAttributesCompression, fPackageAttributesCompressedLength,
			fPackageAttributesUncompressedLength)) {
		fprintf(stderr, "Error: File \"%s\" is not a valid package file: "
			"package attributes section: %s\n", fileName, errorString);
		return B_BAD_DATA;
	}

	// TOC length and compression
	fTOCCompression = B_BENDIAN_TO_HOST_INT32(header.toc_compression);
	fTOCCompressedLength
		= B_BENDIAN_TO_HOST_INT64(header.toc_length_compressed);
	fTOCUncompressedLength
		= B_BENDIAN_TO_HOST_INT64(header.toc_length_uncompressed);

	if (const char* errorString = _CheckCompression(fTOCCompression,
			fTOCCompressedLength, fTOCUncompressedLength)) {
		fprintf(stderr, "Error: File \"%s\" is not a valid package file: "
			"TOC section: %s\n", fileName, errorString);
		return B_BAD_DATA;
	}

	// TOC subsections
	fTOCAttributeTypesLength
		= B_BENDIAN_TO_HOST_INT64(header.toc_attribute_types_length);
	fTOCAttributeTypesCount
		= B_BENDIAN_TO_HOST_INT64(header.toc_attribute_types_count);
	fTOCStringsLength = B_BENDIAN_TO_HOST_INT64(header.toc_strings_length);
	fTOCStringsCount = B_BENDIAN_TO_HOST_INT64(header.toc_strings_count);

	if (fTOCAttributeTypesLength > fTOCUncompressedLength
		|| fTOCStringsLength > fTOCUncompressedLength - fTOCAttributeTypesLength
		|| fTOCAttributeTypesCount > fTOCAttributeTypesLength
		|| fTOCStringsCount > fTOCStringsLength) {
		fprintf(stderr, "Error: File \"%s\" is not a valid package file: "
			"Invalid TOC subsections description\n", fileName);
		return B_BAD_DATA;
	}

	// check whether the sections fit together
	if (fPackageAttributesCompressedLength > fTotalSize
		|| fTOCCompressedLength
			> fTotalSize - fPackageAttributesCompressedLength
		|| fHeapOffset
			> fTotalSize - fPackageAttributesCompressedLength
				- fTOCCompressedLength) {
		fprintf(stderr, "Error: File \"%s\" is not a valid package file: "
			"The sum of the sections sizes is greater than the package size\n",
			fileName);
		return B_BAD_DATA;
	}

	fPackageAttributesOffset = fTotalSize - fPackageAttributesCompressedLength;
	fTOCSectionOffset = fPackageAttributesOffset - fTOCCompressedLength;
	fHeapSize = fTOCSectionOffset - fHeapOffset;

	// TOC size sanity check
	if (fTOCUncompressedLength > kMaxTOCSize) {
		fprintf(stderr, "Error: Package file \"%s\" has a TOC section size of "
			"%llu bytes. This is beyond the reader's sanity limit\n", fileName,
			fTOCUncompressedLength);
		return B_UNSUPPORTED;
	}

	// allocate a scratch buffer
	fScratchBuffer = new(std::nothrow) uint8[kScratchBufferSize];
	if (fScratchBuffer == NULL) {
		fprintf(stderr, "Error: Out of memory!\n");
		return B_NO_MEMORY;
	}
	fScratchBufferSize = kScratchBufferSize;

	// read in the complete TOC
	fTOCSection = new(std::nothrow) uint8[fTOCUncompressedLength];
	if (fTOCSection == NULL) {
		fprintf(stderr, "Error: Out of memory!\n");
		return B_NO_MEMORY;
	}

	error = _ReadCompressedBuffer(fTOCSectionOffset, fTOCSection,
		fTOCCompressedLength, fTOCUncompressedLength, fTOCCompression);
	if (error != B_OK)
		return error;

	// start parsing the TOC
	fCurrentTOCOffset = 0;

	// attribute types
	error = _ParseTOCAttributeTypes();
	if (error != B_OK)
		return error;
	fCurrentTOCOffset += fTOCAttributeTypesLength;

	// strings
	error = _ParseTOCStrings();
	if (error != B_OK)
		return error;
	fCurrentTOCOffset += fTOCStringsLength;

	return B_OK;
}


status_t
PackageReader::ParseContent(PackageContentHandler* contentHandler)
{
	AttributeHandlerContext context(contentHandler);
	RootAttributeHandler rootAttributeHandler;
	return _ParseContent(&context, &rootAttributeHandler);
}


status_t
PackageReader::ParseContent(LowLevelPackageContentHandler* contentHandler)
{
	AttributeHandlerContext context(contentHandler);
	PackageAttributeHandler rootAttributeHandler;
	return _ParseContent(&context, &rootAttributeHandler);
}


const char*
PackageReader::_CheckCompression(uint32 compression, uint64 compressedLength,
	uint64 uncompressedLength) const
{
	switch (compression) {
		case B_HPKG_COMPRESSION_NONE:
			if (compressedLength != uncompressedLength) {
				return "Uncompressed, but compressed and uncompressed length "
					"don't match";
			}
			return NULL;

		case B_HPKG_COMPRESSION_ZLIB:
			if (compressedLength >= uncompressedLength) {
				return "Compressed, but compressed length is not less than "
					"uncompressed length";
			}
			return NULL;

		default:
			return "Invalid compression algorithm ID";
	}
}


status_t
PackageReader::_ParseTOCAttributeTypes()
{
	// allocate table
	fAttributeTypes = new(std::nothrow) AttributeTypeReference[
		fTOCAttributeTypesCount];
	if (fAttributeTypes == NULL) {
		fprintf(stderr, "Error: Out of memory!\n");
		return B_NO_MEMORY;
	}

	// parse the section and fill the table
	uint8* position = fTOCSection + fCurrentTOCOffset;
	uint8* sectionEnd = position + fTOCAttributeTypesLength;
	uint32 index = 0;
	while (true) {
		if (position >= sectionEnd) {
			fprintf(stderr, "Error: Malformed TOC attribute types section\n");
			return B_BAD_DATA;
		}

		AttributeType* type = (AttributeType*)position;

		if (type->type == 0) {
			if (position + 1 != sectionEnd) {
				fprintf(stderr, "Error: Excess bytes in TOC attribute types "
					"section\n");
printf("position: %p, sectionEnd: %p\n", position, sectionEnd);
				return B_BAD_DATA;
			}

			if (index != fTOCAttributeTypesCount) {
				fprintf(stderr, "Error: Invalid TOC attribute types section: "
					"Less types than specified in the header\n");
				return B_BAD_DATA;
			}

			return B_OK;
		}

		if (index >= fTOCAttributeTypesCount) {
			fprintf(stderr, "Error: Invalid TOC attribute types section: "
				"More types than specified in the header\n");
			return B_BAD_DATA;
		}

		size_t nameLength = strnlen(type->name,
			(char*)sectionEnd - type->name);
		position = (uint8*)type->name + nameLength + 1;
		fAttributeTypes[index].type = type;
		fAttributeTypes[index].standardIndex = _GetStandardIndex(type);
		index++;
printf("type: %u, \"%s\"\n", type->type, type->name);
	}
}


status_t
PackageReader::_ParseTOCStrings()
{
	// allocate table
	fStrings = new(std::nothrow) char*[fTOCStringsCount];
	if (fStrings == NULL) {
		fprintf(stderr, "Error: Out of memory!\n");
		return B_NO_MEMORY;
	}

	// parse the section and fill the table
	char* position = (char*)fTOCSection + fCurrentTOCOffset;
	char* sectionEnd = position + fTOCStringsLength;
	uint32 index = 0;
	while (true) {
		if (position >= sectionEnd) {
			fprintf(stderr, "Error: Malformed TOC strings section\n");
			return B_BAD_DATA;
		}

		size_t stringLength = strnlen(position, (char*)sectionEnd - position);

		if (stringLength == 0) {
			if (position + 1 != sectionEnd) {
				fprintf(stderr, "Error: Excess bytes in TOC strings section\n");
printf("position: %p, sectionEnd: %p\n", position, sectionEnd);
				return B_BAD_DATA;
			}

			if (index != fTOCStringsCount) {
				fprintf(stderr, "Error: Invalid TOC strings section: "
					"Less strings than specified in the header\n");
				return B_BAD_DATA;
			}

			return B_OK;
		}

		if (index >= fTOCStringsCount) {
			fprintf(stderr, "Error: Invalid TOC strings section: "
				"More strings than specified in the header\n");
			return B_BAD_DATA;
		}

		fStrings[index++] = position;
printf("string: \"%s\"\n", position);
		position += stringLength + 1;
	}
}


status_t
PackageReader::_ParseContent(AttributeHandlerContext* context,
	AttributeHandler* rootAttributeHandler)
{
	// parse the TOC
	fCurrentTOCOffset = fTOCAttributeTypesLength + fTOCStringsLength;

	// prepare attribute handler context
	context->heapOffset = fHeapOffset;
	context->heapSize = fHeapSize;

	// init the attribute handler stack
	AttributeHandlerList attributeHandlerStack;
	fAttributeHandlerStack = &attributeHandlerStack;
	fAttributeHandlerStack->Add(rootAttributeHandler);
	rootAttributeHandler->SetLevel(0);

	status_t error = _ParseAttributeTree(context);
	if (error == B_OK) {
		if (fCurrentTOCOffset < fTOCUncompressedLength) {
			fprintf(stderr, "Error: %llu excess byte(s) in TOC section\n",
				fTOCUncompressedLength - fCurrentTOCOffset);
			error = B_BAD_DATA;
		}
	}

	// clean up on error
	if (error != B_OK) {
		context->ErrorOccurred();
		while (AttributeHandler* handler = _PopAttributeHandler()) {
			if (handler != rootAttributeHandler)
				handler->Delete(context);
		}
		return error;
	}

	return B_OK;
}


status_t
PackageReader::_ParseAttributeTree(AttributeHandlerContext* context)
{
	int level = 0;

	while (true) {
		uint64 tag;
		status_t error = _ReadUnsignedLEB128(tag);
		if (error != B_OK)
			return error;

		if (tag == 0) {
			AttributeHandler* handler = _PopAttributeHandler();
			if (level-- == 0)
				return B_OK;

			error = handler->Delete(context);
			if (error != B_OK)
				return error;

			continue;
		}

		// get the type
		uint64 typeIndex = B_HPKG_ATTRIBUTE_TAG_INDEX(tag);
		if (typeIndex >= fTOCAttributeTypesCount) {
			fprintf(stderr, "Error: Invalid TOC section: "
				"Invalid attribute type index\n");
			return B_BAD_DATA;
		}
		AttributeTypeReference* type = fAttributeTypes + typeIndex;

		// get the value
		AttributeValue value;
		error = _ReadAttributeValue(type->type->type,
			B_HPKG_ATTRIBUTE_TAG_ENCODING(tag), value);
		if (error != B_OK)
			return error;

		bool hasChildren = B_HPKG_ATTRIBUTE_TAG_HAS_CHILDREN(tag);
		AttributeHandler* childHandler = NULL;
		error = _CurrentAttributeHandler()->HandleChildAttribute(context,
			type->type, type->standardIndex, value,
			hasChildren ? &childHandler : NULL);
		if (error != B_OK)
			return error;

		// parse children
		if (hasChildren) {
			// create an ignore handler, if necessary
			if (childHandler == NULL) {
				childHandler = new(std::nothrow) IgnoreAttributeHandler;
				if (childHandler == NULL) {
					fprintf(stderr, "Error: Out of memory!\n");
					return B_NO_MEMORY;
				}
			}

			childHandler->SetLevel(level);
			_PushAttributeHandler(childHandler);
			level++;
		}
	}
}


status_t
PackageReader::_ReadAttributeValue(uint8 type, uint8 encoding,
	AttributeValue& _value)
{
	switch (type) {
		case B_HPKG_ATTRIBUTE_TYPE_INT:
		case B_HPKG_ATTRIBUTE_TYPE_UINT:
		{
			uint64 intValue;
			status_t error;

			switch (encoding) {
				case B_HPKG_ATTRIBUTE_ENCODING_INT_8_BIT:
				{
					uint8 value;
					error = _Read(value);
					intValue = value;
					break;
				}
				case B_HPKG_ATTRIBUTE_ENCODING_INT_16_BIT:
				{
					uint16 value;
					error = _Read(value);
					intValue = B_BENDIAN_TO_HOST_INT16(value);
					break;
				}
				case B_HPKG_ATTRIBUTE_ENCODING_INT_32_BIT:
				{
					uint32 value;
					error = _Read(value);
					intValue = B_BENDIAN_TO_HOST_INT32(value);
					break;
				}
				case B_HPKG_ATTRIBUTE_ENCODING_INT_64_BIT:
				{
					uint64 value;
					error = _Read(value);
					intValue = B_BENDIAN_TO_HOST_INT64(value);
					break;
				}
				default:
				{
					fprintf(stderr, "Error: Invalid TOC section: invalid "
						"encoding %d for int value type %d\n", encoding, type);
					throw status_t(B_BAD_VALUE);
				}
			}

			if (error != B_OK)
				return error;

			if (type == B_HPKG_ATTRIBUTE_TYPE_INT)
				_value.SetTo((int64)intValue);
			else
				_value.SetTo(intValue);

			return B_OK;
		}

		case B_HPKG_ATTRIBUTE_TYPE_STRING:
		{
			if (encoding == B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE) {
				uint64 index;
				status_t error = _ReadUnsignedLEB128(index);
				if (error != B_OK)
					return error;

				if (index > fTOCStringsCount) {
					fprintf(stderr, "Error: Invalid TOC section: string "
						"reference out of bounds\n");
					return B_BAD_DATA;
				}

				_value.SetTo(fStrings[index]);
			} else if (encoding == B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE) {
				const char* string;
				status_t error = _ReadString(string);
				if (error != B_OK)
					return error;

				_value.SetTo(string);
			} else {
				fprintf(stderr, "Error: Invalid TOC section: invalid string "
					"encoding (%u)\n", encoding);
				return B_BAD_DATA;
			}

			return B_OK;
		}

		case B_HPKG_ATTRIBUTE_TYPE_RAW:
		{
			uint64 size;
			status_t error = _ReadUnsignedLEB128(size);
			if (error != B_OK)
				return error;

			if (encoding == B_HPKG_ATTRIBUTE_ENCODING_RAW_HEAP) {
				uint64 offset;
				error = _ReadUnsignedLEB128(offset);
				if (error != B_OK)
					return error;

				if (offset > fHeapSize || size > fHeapSize - offset) {
					fprintf(stderr, "Error: Invalid TOC section: invalid data "
						"reference\n");
					return B_BAD_DATA;
				}

				_value.SetToData(size, fHeapOffset + offset);
			} else if (encoding == B_HPKG_ATTRIBUTE_ENCODING_RAW_INLINE) {
				if (size > B_HPKG_MAX_INLINE_DATA_SIZE) {
					fprintf(stderr, "Error: Invalid TOC section: inline data "
						"too long\n");
					return B_BAD_DATA;
				}

				const void* buffer;
				error = _GetTOCBuffer(size, buffer);
				if (error != B_OK)
					return error;
				_value.SetToData(size, buffer);
			} else {
				fprintf(stderr, "Error: Invalid TOC section: invalid raw "
					"encoding (%u)\n", encoding);
				return B_BAD_DATA;
			}

			return B_OK;
		}

		default:
			fprintf(stderr, "Error: Invalid TOC section: invalid value type: "
				"%d\n", type);
			return B_BAD_DATA;
	}
}


status_t
PackageReader::_ReadUnsignedLEB128(uint64& _value)
{
	uint64 result = 0;
	int shift = 0;
	while (true) {
		uint8 byte;
		status_t error = _Read(byte);
		if (error != B_OK)
			return error;

		result |= uint64(byte & 0x7f) << shift;
		if ((byte & 0x80) == 0)
			break;
		shift += 7;
	}

	_value = result;
	return B_OK;
}


status_t
PackageReader::_ReadString(const char*& _string, size_t* _stringLength)
{
	const char* string = (const char*)fTOCSection + fCurrentTOCOffset;
	size_t stringLength = strnlen(string,
		fTOCUncompressedLength - fCurrentTOCOffset);

	if (stringLength == fTOCUncompressedLength - fCurrentTOCOffset) {
		fprintf(stderr, "_ReadString(): string extends beyond TOC end\n");
		return B_BAD_DATA;
	}

	_string = string;
	if (_stringLength != NULL)
		*_stringLength = stringLength;

	fCurrentTOCOffset += stringLength + 1;
	return B_OK;
}


status_t
PackageReader::_GetTOCBuffer(size_t size, const void*& _buffer)
{
	if (size > fTOCUncompressedLength - fCurrentTOCOffset) {
		fprintf(stderr, "_GetTOCBuffer(%lu): read beyond TOC end\n", size);
		return B_BAD_DATA;
	}

	_buffer = fTOCSection + fCurrentTOCOffset;
	fCurrentTOCOffset += size;
	return B_OK;
}


status_t
PackageReader::_ReadTOCBuffer(void* buffer, size_t size)
{
	if (size > fTOCUncompressedLength - fCurrentTOCOffset) {
		fprintf(stderr, "_ReadTOCBuffer(%lu): read beyond TOC end\n", size);
		return B_BAD_DATA;
	}

	memcpy(buffer, fTOCSection + fCurrentTOCOffset, size);
	fCurrentTOCOffset += size;
	return B_OK;
}


status_t
PackageReader::_ReadBuffer(off_t offset, void* buffer, size_t size)
{
	ssize_t bytesRead = pread(fFD, buffer, size, offset);
	if (bytesRead < 0) {
		fprintf(stderr, "_ReadBuffer(%p, %lu) failed to read data: %s\n",
			buffer, size, strerror(errno));
		return errno;
	}
	if ((size_t)bytesRead != size) {
		fprintf(stderr, "_ReadBuffer(%p, %lu) failed to read all data\n",
			buffer, size);
		return B_ERROR;
	}

	return B_OK;
}


status_t
PackageReader::_ReadCompressedBuffer(off_t offset, void* buffer,
	size_t compressedSize, size_t uncompressedSize, uint32 compression)
{
	switch (compression) {
		case B_HPKG_COMPRESSION_NONE:
			return _ReadBuffer(offset, buffer, compressedSize);

		case B_HPKG_COMPRESSION_ZLIB:
		{
			// init the decompressor
			BufferDataOutput bufferOutput(buffer, uncompressedSize);
			ZlibDecompressor decompressor(&bufferOutput);
			status_t error = decompressor.Init();
			if (error != B_OK)
				return error;

			while (compressedSize > 0) {
				// read compressed buffer
				size_t toRead = std::min(compressedSize, fScratchBufferSize);
				error = _ReadBuffer(offset, fScratchBuffer, toRead);
				if (error != B_OK)
					return error;

				// uncompress
				error = decompressor.DecompressNext(fScratchBuffer, toRead);
				if (error != B_OK)
					return error;

				compressedSize -= toRead;
				offset += toRead;
			}

			error = decompressor.Finish();
			if (error != B_OK)
				return error;

			// verify that all data have been read
			if (bufferOutput.BytesWritten() != uncompressedSize) {
				fprintf(stderr, "Error: Missing bytes in uncompressed "
					"buffer!\n");
				return B_BAD_DATA;
			}

			return B_OK;
		}

		default:
			return B_BAD_DATA;
	}
}


/*static*/ int8
PackageReader::_GetStandardIndex(const AttributeType* type)
{
	// TODO: Building a hash table once would make the loopup faster.
	for (int32 i = 0; kStandardAttributeIndices[i].name != NULL; i++) {
		if (type->type == kStandardAttributeIndices[i].type
			&& strcmp(kStandardAttributeIndices[i].name, type->name) == 0) {
			return i;
		}
	}

	return -1;
}
