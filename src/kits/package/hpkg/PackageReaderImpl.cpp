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

#include <package/hpkg/haiku_package.h>

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

static const size_t kScratchBufferSize			= 64 * 1024;


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
	{ HPKG_ATTRIBUTE_NAME_##name, B_HPKG_ATTRIBUTE_TYPE_##type, \
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


// #pragma mark - AttributeType


struct PackageReaderImpl::AttributeType {
	uint8	type;
	char	name[0];
};


// #pragma mark - AttributeType


struct PackageReaderImpl::AttributeTypeReference {
	AttributeType*	type;
	int32			standardIndex;
};


// #pragma mark - AttributeHandler


struct PackageReaderImpl::AttributeHandlerContext {
	BErrorOutput*	errorOutput;
	union {
		BPackageContentHandler*			packageContentHandler;
		BLowLevelPackageContentHandler*	lowLevelPackageContentHandler;
	};
	bool					hasLowLevelHandler;

	uint64					heapOffset;
	uint64					heapSize;


	AttributeHandlerContext(BErrorOutput* errorOutput,
		BPackageContentHandler* packageContentHandler)
		:
		errorOutput(errorOutput),
		packageContentHandler(packageContentHandler),
		hasLowLevelHandler(false)
	{
	}

	AttributeHandlerContext(BErrorOutput* errorOutput,
		BLowLevelPackageContentHandler* lowLevelPackageContentHandler)
		:
		errorOutput(errorOutput),
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


struct PackageReaderImpl::AttributeHandler
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
TRACE("%*signored attribute \"%s\" (%u)\n", fLevel * 2, "", type->name, type->type);
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


struct PackageReaderImpl::IgnoreAttributeHandler : AttributeHandler {
};


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
						context->errorOutput->PrintError("Error: Invalid "
							"compression type for data (%llu)\n",
							value.unsignedInt);
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
	BPackageData*	fData;
};


struct PackageReaderImpl::AttributeAttributeHandler : AttributeHandler {
	AttributeAttributeHandler(BPackageEntry* entry, const char* name)
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
	BPackageEntry*			fEntry;
	BPackageEntryAttribute	fAttribute;
};


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

//TRACE("%*sentry \"%s\"\n", fLevel * 2, "", value.string);
				if (_handler != NULL) {
					return EntryAttributeHandler::Create(context, &fEntry,
						value.string, *_handler);
				}
				return B_OK;
			}

			case ATTRIBUTE_INDEX_FILE_TYPE:
				return _SetFileType(context, value.unsignedInt);

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
					BPackageEntryAttribute attribute(value.string);
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
	BPackageEntry		fEntry;
	bool				fNotified;
};


struct PackageReaderImpl::RootAttributeHandler : AttributeHandler {
	virtual status_t HandleChildAttribute(AttributeHandlerContext* context,
		AttributeType* type, int8 typeIndex, const AttributeValue& value,
		AttributeHandler** _handler)
	{
		if (typeIndex == ATTRIBUTE_INDEX_DIRECTORY_ENTRY) {
//TRACE("%*sentry \"%s\"\n", fLevel * 2, "", value.string);
			if (_handler != NULL) {
				return EntryAttributeHandler::Create(context, NULL,
					value.string, *_handler);
			}
			return B_OK;
		}

		return AttributeHandler::HandleChildAttribute(context, type, typeIndex,
			value, _handler);
	}
};


struct PackageReaderImpl::PackageAttributeHandler : AttributeHandler {
	PackageAttributeHandler()
		:
		fToken(NULL),
		fAttributeName(NULL)
	{
	}

	PackageAttributeHandler(const char* attributeName,
		const BPackageAttributeValue& value, void* token)
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


// #pragma mark - PackageReaderImpl


inline PackageReaderImpl::AttributeHandler*
PackageReaderImpl::_CurrentAttributeHandler() const
{
	return fAttributeHandlerStack->Head();
}


inline void
PackageReaderImpl::_PushAttributeHandler(AttributeHandler* handler)
{
	fAttributeHandlerStack->Add(handler);
}


inline PackageReaderImpl::AttributeHandler*
PackageReaderImpl::_PopAttributeHandler()
{
	return fAttributeHandlerStack->RemoveHead();
}


PackageReaderImpl::PackageReaderImpl(BErrorOutput* errorOutput)
	:
	fErrorOutput(errorOutput),
	fFD(-1),
	fOwnsFD(false),
	fTOCSection("TOC"),
	fPackageAttributesSection("package attributes section"),
	fCurrentSection(NULL),
	fAttributeTypes(NULL),
	fScratchBuffer(NULL),
	fScratchBufferSize(0)
{
}


PackageReaderImpl::~PackageReaderImpl()
{
	if (fOwnsFD && fFD >= 0)
		close(fFD);

	delete[] fScratchBuffer;
	delete[] fAttributeTypes;
}


status_t
PackageReaderImpl::Init(const char* fileName)
{
	// open file
	int fd = open(fileName, O_RDONLY);
	if (fd < 0) {
		fErrorOutput->PrintError("Error: Failed to open package file \"%s\": "
			"%s\n", fileName, strerror(errno));
		return errno;
	}

	return Init(fd, true);
}


status_t
PackageReaderImpl::Init(int fd, bool keepFD)
{
	fFD = fd;
	fOwnsFD = keepFD;

	// stat it
	struct stat st;
	if (fstat(fFD, &st) < 0) {
		fErrorOutput->PrintError("Error: Failed to access package file: %s\n",
			strerror(errno));
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
		fErrorOutput->PrintError("Error: Invalid package file: Invalid "
			"magic\n");
		return B_BAD_DATA;
	}

	// header size
	fHeapOffset = B_BENDIAN_TO_HOST_INT16(header.header_size);
	if ((size_t)fHeapOffset < sizeof(hpkg_header)) {
		fErrorOutput->PrintError("Error: Invalid package file: Invalid header "
			"size (%llu)\n", fHeapOffset);
		return B_BAD_DATA;
	}

	// version
	if (B_BENDIAN_TO_HOST_INT16(header.version) != B_HPKG_VERSION) {
		fErrorOutput->PrintError("Error: Invalid/unsupported package file "
			"version (%d)\n", B_BENDIAN_TO_HOST_INT16(header.version));
		return B_BAD_DATA;
	}

	// total size
	fTotalSize = B_BENDIAN_TO_HOST_INT64(header.total_size);
	if (fTotalSize != (uint64)st.st_size) {
		fErrorOutput->PrintError("Error: Invalid package file: Total size in "
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

	if (const char* errorString = _CheckCompression(
		fPackageAttributesSection)) {
		fErrorOutput->PrintError("Error: Invalid package file: package "
			"attributes section: %s\n", errorString);
		return B_BAD_DATA;
	}

	// TOC length and compression
	fTOCSection.compression = B_BENDIAN_TO_HOST_INT32(header.toc_compression);
	fTOCSection.compressedLength
		= B_BENDIAN_TO_HOST_INT64(header.toc_length_compressed);
	fTOCSection.uncompressedLength
		= B_BENDIAN_TO_HOST_INT64(header.toc_length_uncompressed);

	if (const char* errorString = _CheckCompression(fTOCSection)) {
		fErrorOutput->PrintError("Error: Invalid package file: TOC section: "
			"%s\n", errorString);
		return B_BAD_DATA;
	}

	// TOC subsections
	fTOCAttributeTypesLength
		= B_BENDIAN_TO_HOST_INT64(header.toc_attribute_types_length);
	fTOCAttributeTypesCount
		= B_BENDIAN_TO_HOST_INT64(header.toc_attribute_types_count);
	fTOCSection.stringsLength
		= B_BENDIAN_TO_HOST_INT64(header.toc_strings_length);
	fTOCSection.stringsCount
		= B_BENDIAN_TO_HOST_INT64(header.toc_strings_count);

	if (fTOCAttributeTypesLength > fTOCSection.uncompressedLength
		|| fTOCSection.stringsLength
			> fTOCSection.uncompressedLength - fTOCAttributeTypesLength
		|| fTOCAttributeTypesCount > fTOCAttributeTypesLength
		|| fTOCSection.stringsCount > fTOCSection.stringsLength) {
		fErrorOutput->PrintError("Error: Invalid package file: Invalid TOC "
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
		fErrorOutput->PrintError("Error: Invalid package file: The sum of the "
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
		fErrorOutput->PrintError("Error: Package file TOC section size "
			"is %llu bytes. This is beyond the reader's sanity limit\n",
			fTOCSection.uncompressedLength);
		return B_UNSUPPORTED;
	}

	// package attributes size sanity check
	if (fPackageAttributesSection.uncompressedLength
			> kMaxPackageAttributesSize) {
		fErrorOutput->PrintError(
			"Error: Package file package attributes section size "
			"is %llu bytes. This is beyond the reader's sanity limit\n",
			fPackageAttributesSection.uncompressedLength);
		return B_UNSUPPORTED;
	}

	// allocate a scratch buffer
	fScratchBuffer = new(std::nothrow) uint8[kScratchBufferSize];
	if (fScratchBuffer == NULL) {
		fErrorOutput->PrintError("Error: Out of memory!\n");
		return B_NO_MEMORY;
	}
	fScratchBufferSize = kScratchBufferSize;

	// read in the complete TOC
	fTOCSection.data
		= new(std::nothrow) uint8[fTOCSection.uncompressedLength];
	if (fTOCSection.data == NULL) {
		fErrorOutput->PrintError("Error: Out of memory!\n");
		return B_NO_MEMORY;
	}
	error = _ReadCompressedBuffer(fTOCSection);
	if (error != B_OK)
		return error;

	// read in the complete package attributes section
	fPackageAttributesSection.data
		= new(std::nothrow) uint8[fPackageAttributesSection.uncompressedLength];
	if (fPackageAttributesSection.data == NULL) {
		fErrorOutput->PrintError("Error: Out of memory!\n");
		return B_NO_MEMORY;
	}
	error = _ReadCompressedBuffer(fPackageAttributesSection);
	if (error != B_OK)
		return error;

	// start parsing the TOC
	fCurrentSection = &fTOCSection;
	fCurrentSection->currentOffset = 0;

	// attribute types
	error = _ParseTOCAttributeTypes();
	if (error != B_OK)
		return error;
	fCurrentSection->currentOffset += fTOCAttributeTypesLength;

	// strings
	error = _ParseStrings();
	if (error != B_OK)
		return error;
	fCurrentSection->currentOffset += fCurrentSection->stringsLength;

	return B_OK;
}


status_t
PackageReaderImpl::ParseContent(BPackageContentHandler* contentHandler)
{
	AttributeHandlerContext context(fErrorOutput, contentHandler);
	RootAttributeHandler rootAttributeHandler;

	status_t result = _ParseContent(&context, &rootAttributeHandler);
	if (result != B_OK)
		return result;

	fCurrentSection = &fPackageAttributesSection;
	fCurrentSection->currentOffset = 0;

	// strings
	status_t error = _ParseStrings();
	if (error != B_OK)
		return error;
	fCurrentSection->currentOffset += fCurrentSection->stringsLength;

	return _ParsePackageAttributes(&context);
}


status_t
PackageReaderImpl::ParseContent(BLowLevelPackageContentHandler* contentHandler)
{
	AttributeHandlerContext context(fErrorOutput, contentHandler);
	PackageAttributeHandler rootAttributeHandler;
	return _ParseContent(&context, &rootAttributeHandler);
}


const char*
PackageReaderImpl::_CheckCompression(const SectionInfo& section) const
{
	switch (section.compression) {
		case B_HPKG_COMPRESSION_NONE:
			if (section.compressedLength != section.uncompressedLength) {
				return "Uncompressed, but compressed and uncompressed length "
					"don't match";
			}
			return NULL;

		case B_HPKG_COMPRESSION_ZLIB:
			if (section.compressedLength >= section.uncompressedLength) {
				return "Compressed, but compressed length is not less than "
					"uncompressed length";
			}
			return NULL;

		default:
			return "Invalid compression algorithm ID";
	}
}


status_t
PackageReaderImpl::_ParseTOCAttributeTypes()
{
	// allocate table
	fAttributeTypes = new(std::nothrow) AttributeTypeReference[
		fTOCAttributeTypesCount];
	if (fAttributeTypes == NULL) {
		fErrorOutput->PrintError("Error: Out of memory!\n");
		return B_NO_MEMORY;
	}

	// parse the section and fill the table
	uint8* position = fTOCSection.data + fTOCSection.currentOffset;
	uint8* sectionEnd = position + fTOCAttributeTypesLength;
	uint32 index = 0;
	while (true) {
		if (position >= sectionEnd) {
			fErrorOutput->PrintError("Error: Malformed TOC attribute types "
				"section\n");
			return B_BAD_DATA;
		}

		AttributeType* type = (AttributeType*)position;

		if (type->type == 0) {
			if (position + 1 != sectionEnd) {
				fErrorOutput->PrintError("Error: Excess bytes in TOC attribute "
					"types section\n");
				return B_BAD_DATA;
			}

			if (index != fTOCAttributeTypesCount) {
				fErrorOutput->PrintError("Error: Invalid TOC attribute types "
					"section: Less types than specified in the header\n");
				return B_BAD_DATA;
			}

			return B_OK;
		}

		if (index >= fTOCAttributeTypesCount) {
			fErrorOutput->PrintError("Error: Invalid TOC attribute types "
				"section: More types than specified in the header\n");
			return B_BAD_DATA;
		}

		size_t nameLength = strnlen(type->name,
			(char*)sectionEnd - type->name);
		position = (uint8*)type->name + nameLength + 1;
		fAttributeTypes[index].type = type;
		fAttributeTypes[index].standardIndex = _GetStandardIndex(type);
		index++;
	}
}


status_t
PackageReaderImpl::_ParseStrings()
{
	// allocate table
	fCurrentSection->strings
		= new(std::nothrow) char*[fCurrentSection->stringsCount];
	if (fCurrentSection->strings == NULL) {
		fErrorOutput->PrintError("Error: Out of memory!\n");
		return B_NO_MEMORY;
	}

	// parse the section and fill the table
	char* position
		= (char*)fCurrentSection->data + fCurrentSection->currentOffset;
	char* sectionEnd = position + fCurrentSection->stringsLength;
	uint32 index = 0;
	while (true) {
		if (position >= sectionEnd) {
			fErrorOutput->PrintError("Error: Malformed %s strings section\n",
				fCurrentSection->name);
			return B_BAD_DATA;
		}

		size_t stringLength = strnlen(position, (char*)sectionEnd - position);

		if (stringLength == 0) {
			if (position + 1 != sectionEnd) {
				fErrorOutput->PrintError(
					"Error: %ld excess bytes in %s strings section\n",
					sectionEnd - (position + 1), fCurrentSection->name);
				return B_BAD_DATA;
			}

			if (index != fCurrentSection->stringsCount) {
				fErrorOutput->PrintError("Error: Invalid %s strings section: "
					"Less strings (%lld) than specified in the header (%lld)\n",
					fCurrentSection->name, index,
					fCurrentSection->stringsCount);
				return B_BAD_DATA;
			}

			return B_OK;
		}

		if (index >= fCurrentSection->stringsCount) {
			fErrorOutput->PrintError("Error: Invalid %s strings section: "
				"More strings (%lld) than specified in the header (%lld)\n",
				fCurrentSection->name, index, fCurrentSection->stringsCount);
			return B_BAD_DATA;
		}

		fCurrentSection->strings[index++] = position;
		position += stringLength + 1;
	}
}


status_t
PackageReaderImpl::_ParseContent(AttributeHandlerContext* context,
	AttributeHandler* rootAttributeHandler)
{
	// parse the TOC
	fTOCSection.currentOffset = fTOCAttributeTypesLength
		+ fTOCSection.stringsLength;

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
		if (fTOCSection.currentOffset < fTOCSection.uncompressedLength) {
			fErrorOutput->PrintError("Error: %llu excess byte(s) in TOC "
				"section\n",
				fTOCSection.uncompressedLength - fTOCSection.currentOffset);
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
PackageReaderImpl::_ParseAttributeTree(AttributeHandlerContext* context)
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
		uint64 typeIndex = HPKG_ATTRIBUTE_TAG_INDEX(tag);
		if (typeIndex >= fTOCAttributeTypesCount) {
			fErrorOutput->PrintError("Error: Invalid TOC section: "
				"Invalid attribute type index\n");
			return B_BAD_DATA;
		}
		AttributeTypeReference* type = fAttributeTypes + typeIndex;

		// get the value
		AttributeValue value;
		error = _ReadAttributeValue(type->type->type,
			HPKG_ATTRIBUTE_TAG_ENCODING(tag), value);
		if (error != B_OK)
			return error;

		bool hasChildren = HPKG_ATTRIBUTE_TAG_HAS_CHILDREN(tag);
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
					fErrorOutput->PrintError("Error: Out of memory!\n");
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
PackageReaderImpl::_ParsePackageAttributes(AttributeHandlerContext* context)
{
	while (true) {
		uint8 id;
		AttributeValue attributeValue;
		bool hasChildren;
		uint64 tag;
		BPackageInfoAttributeValue handlerValue;

		status_t error
			= _ReadPackageAttribute(id, attributeValue, &hasChildren, &tag);
		if (error != B_OK)
			return error;

		if (tag == 0)
			return B_OK;

		switch (id) {
			case HPKG_PACKAGE_ATTRIBUTE_NAME:
				handlerValue.SetTo(B_PACKAGE_INFO_NAME, attributeValue.string);
				break;

			case HPKG_PACKAGE_ATTRIBUTE_SUMMARY:
				handlerValue.SetTo(B_PACKAGE_INFO_SUMMARY,
					attributeValue.string);
				break;

			case HPKG_PACKAGE_ATTRIBUTE_DESCRIPTION:
				handlerValue.SetTo(B_PACKAGE_INFO_DESCRIPTION,
					attributeValue.string);
				break;

			case HPKG_PACKAGE_ATTRIBUTE_VENDOR:
				handlerValue.SetTo(B_PACKAGE_INFO_VENDOR,
					attributeValue.string);
				break;

			case HPKG_PACKAGE_ATTRIBUTE_PACKAGER:
				handlerValue.SetTo(B_PACKAGE_INFO_PACKAGER,
					attributeValue.string);
				break;

			case HPKG_PACKAGE_ATTRIBUTE_ARCHITECTURE:
				if (attributeValue.unsignedInt
						>= B_PACKAGE_ARCHITECTURE_ENUM_COUNT) {
					fErrorOutput->PrintError(
						"Error: Invalid package attribute section: "
						"Invalid package architecture %lld encountered\n",
						attributeValue.unsignedInt);
					return B_BAD_DATA;
				}
				handlerValue.SetTo(B_PACKAGE_INFO_ARCHITECTURE,
					(uint8)attributeValue.unsignedInt);
				break;

			case HPKG_PACKAGE_ATTRIBUTE_VERSION_MAJOR:
				error = _ParsePackageVersion(handlerValue.version,
					attributeValue.string);
				if (error != B_OK)
					return error;
				handlerValue.attributeIndex = B_PACKAGE_INFO_VERSION;
				break;

			case HPKG_PACKAGE_ATTRIBUTE_COPYRIGHT:
				handlerValue.SetTo(B_PACKAGE_INFO_COPYRIGHTS,
					attributeValue.string);
				break;

			case HPKG_PACKAGE_ATTRIBUTE_LICENSE:
				handlerValue.SetTo(B_PACKAGE_INFO_LICENSES,
					attributeValue.string);
				break;

			case HPKG_PACKAGE_ATTRIBUTE_PROVIDES_TYPE:
				error = _ParsePackageProvides(handlerValue.resolvable,
					(BPackageResolvableType)attributeValue.unsignedInt);
				if (error != B_OK)
					return error;
				handlerValue.attributeIndex = B_PACKAGE_INFO_PROVIDES;
				break;

			case HPKG_PACKAGE_ATTRIBUTE_REQUIRES:
				error = _ParsePackageResolvableExpression(
					handlerValue.resolvableExpression, attributeValue.string,
					hasChildren);
				if (error != B_OK)
					return error;
				handlerValue.attributeIndex = B_PACKAGE_INFO_REQUIRES;
				break;

			case HPKG_PACKAGE_ATTRIBUTE_SUPPLEMENTS:
				error = _ParsePackageResolvableExpression(
					handlerValue.resolvableExpression, attributeValue.string,
					hasChildren);
				if (error != B_OK)
					return error;
				handlerValue.attributeIndex = B_PACKAGE_INFO_SUPPLEMENTS;
				break;

			case HPKG_PACKAGE_ATTRIBUTE_CONFLICTS:
				error = _ParsePackageResolvableExpression(
					handlerValue.resolvableExpression, attributeValue.string,
					hasChildren);
				if (error != B_OK)
					return error;
				handlerValue.attributeIndex = B_PACKAGE_INFO_CONFLICTS;
				break;

			case HPKG_PACKAGE_ATTRIBUTE_FRESHENS:
				error = _ParsePackageResolvableExpression(
					handlerValue.resolvableExpression, attributeValue.string,
					hasChildren);
				if (error != B_OK)
					return error;
				handlerValue.attributeIndex = B_PACKAGE_INFO_FRESHENS;
				break;

			case HPKG_PACKAGE_ATTRIBUTE_REPLACES:
				handlerValue.SetTo(B_PACKAGE_INFO_REPLACES,
					attributeValue.string);
				break;

			default:
				fErrorOutput->PrintError(
					"Error: Invalid package attribute section: unexpected "
					"package attribute id %d encountered\n", id);
				return B_BAD_DATA;
		}

		error = context->packageContentHandler->HandlePackageAttribute(
			handlerValue);
		if (error != B_OK)
			return error;
	}
}


status_t
PackageReaderImpl::_ParsePackageVersion(BPackageVersionData& _version,
	const char* major)
{
	uint8 id;
	AttributeValue value;
	status_t error;

	_version.major = NULL;
	_version.minor = NULL;
	_version.micro = NULL;
	_version.release = 0;

	// major (may have been read already)
	if (major == NULL) {
		error = _ReadPackageAttribute(id, value);
		if (error != B_OK)
			return error;
		if (id != HPKG_PACKAGE_ATTRIBUTE_VERSION_MAJOR) {
			fErrorOutput->PrintError("Error: Invalid package attribute section:"
				" Invalid package version, expected major\n");
			return B_BAD_DATA;
		}
		major = value.string;
	}
	_version.major = major;

	// minor
	error = _ReadPackageAttribute(id, value);
	if (error != B_OK)
		return error;
	if (id != HPKG_PACKAGE_ATTRIBUTE_VERSION_MINOR) {
		fErrorOutput->PrintError("Error: Invalid package attribute section:"
			" Invalid package version, expected minor\n");
		return B_BAD_DATA;
	}
	_version.minor = value.string;

	// micro
	error = _ReadPackageAttribute(id, value);
	if (error != B_OK)
		return error;
	if (id != HPKG_PACKAGE_ATTRIBUTE_VERSION_MICRO) {
		fErrorOutput->PrintError("Error: Invalid package attribute section:"
			" Invalid package version, expected micro\n");
		return B_BAD_DATA;
	}
	_version.micro = value.string;

	// release
	error = _ReadPackageAttribute(id, value);
	if (error != B_OK)
		return error;
	if (id != HPKG_PACKAGE_ATTRIBUTE_VERSION_RELEASE) {
		fErrorOutput->PrintError("Error: Invalid package attribute section:"
			" Invalid package version, expected release\n");
		return B_BAD_DATA;
	}
	_version.release = (uint8)value.unsignedInt;

	return B_OK;
}


status_t
PackageReaderImpl::_ParsePackageProvides(BPackageResolvableData& _resolvable,
	BPackageResolvableType providesType)
{
	uint8 id;
	AttributeValue value;
	bool hasChildren;

	if (providesType >= B_PACKAGE_RESOLVABLE_TYPE_ENUM_COUNT) {
		fErrorOutput->PrintError("Error: Invalid package attribute section: "
			"Invalid package provides type %lld encountered\n", providesType);
		return B_BAD_DATA;
	}
	_resolvable.type = providesType;
	_resolvable.haveVersion = false;

	// provides name
	status_t error = _ReadPackageAttribute(id, value, &hasChildren);
	if (error != B_OK)
		return error;
	if (id != HPKG_PACKAGE_ATTRIBUTE_PROVIDES) {
		fErrorOutput->PrintError("Error: Invalid package attribute section: "
			"Invalid package provides, expected name of resolvable\n");
		return B_BAD_DATA;
	}
	_resolvable.name = value.string;

	// there may be a version added as child
	if (hasChildren) {
		error = _ParsePackageVersion(_resolvable.version);
		if (error != B_OK)
			return error;

		_resolvable.haveVersion = true;

		uint64 tag;
		if ((error = _ReadUnsignedLEB128(tag)) != B_OK)
			return error;
		if (tag != 0) {
			fErrorOutput->PrintError("Error: Invalid package attribute "
				"section: Invalid package provides, expected end-tag\n");
			return B_BAD_DATA;
		}
	}

	return B_OK;
}


status_t
PackageReaderImpl::_ParsePackageResolvableExpression(
	BPackageResolvableExpressionData& _resolvableExpression,
	const char* resolvableName, bool hasChildren)
{
	_resolvableExpression.name = resolvableName;
	_resolvableExpression.haveOpAndVersion = false;

	// there may be an operator and a version added as child
	if (hasChildren) {
		// resolvable-expression operator
		uint8 id;
		AttributeValue value;
		status_t error = _ReadPackageAttribute(id, value);
		if (error != B_OK)
			return error;
		if (id != HPKG_PACKAGE_ATTRIBUTE_RESOLVABLE_OPERATOR) {
			fErrorOutput->PrintError("Error: Invalid package attribute section:"
				" Invalid package resolvable expression, expected operator\n");
			return B_BAD_DATA;
		}

		if (value.unsignedInt >= B_PACKAGE_RESOLVABLE_OP_ENUM_COUNT) {
			fErrorOutput->PrintError("Error: Invalid package attribute section:"
				" Invalid package resolvable operator %lld encountered\n",
				value.unsignedInt);
			return B_BAD_DATA;
		}

		_resolvableExpression.op
			= (BPackageResolvableOperator)value.unsignedInt;

		error = _ParsePackageVersion(_resolvableExpression.version);
		if (error != B_OK)
			return error;

		_resolvableExpression.haveOpAndVersion = true;

		uint64 tag;
		if ((error = _ReadUnsignedLEB128(tag)) != B_OK)
			return error;
		if (tag != 0) {
			fErrorOutput->PrintError("Error: Invalid package attribute section:"
				" Invalid package resolvable expression, expected end-tag\n");
			return B_BAD_DATA;
		}
	}

	return B_OK;
}


status_t
PackageReaderImpl::_ReadPackageAttribute(uint8& _id, AttributeValue& _value,
	bool* _hasChildren, uint64* _tag)
{
	uint64 tag;
	status_t error = _ReadUnsignedLEB128(tag);
	if (error != B_OK)
		return error;

	if (tag != 0) {
		// get the type
		uint16 type = HPKG_PACKAGE_ATTRIBUTE_TAG_TYPE(tag);
		if (type >= B_HPKG_ATTRIBUTE_TYPE_ENUM_COUNT) {
			fErrorOutput->PrintError("Error: Invalid package attribute "
				"section: attribute type %d not supported!\n", type);
			return B_BAD_DATA;
		}

		// get the value
		error = _ReadAttributeValue(type,
			HPKG_PACKAGE_ATTRIBUTE_TAG_ENCODING(tag), _value);
		if (error != B_OK)
			return error;

		_id = HPKG_PACKAGE_ATTRIBUTE_TAG_ID(tag);
		if (_id >= HPKG_PACKAGE_ATTRIBUTE_ENUM_COUNT) {
			fErrorOutput->PrintError("Error: Invalid package attribute section: "
				"attribute id %d not supported!\n", _id);
			return B_BAD_DATA;
		}
	}

	if (_hasChildren != NULL)
		*_hasChildren = HPKG_PACKAGE_ATTRIBUTE_TAG_HAS_CHILDREN(tag);
	if (_tag != NULL)
		*_tag = tag;

	return B_OK;
}


status_t
PackageReaderImpl::_ReadAttributeValue(uint8 type, uint8 encoding,
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
					fErrorOutput->PrintError("Error: Invalid %s section: "
						"invalid encoding %d for int value type %d\n",
						fCurrentSection->name, encoding, type);
					return B_BAD_VALUE;
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

				if (index > fCurrentSection->stringsCount) {
					fErrorOutput->PrintError("Error: Invalid %s section: "
						"string reference (%lld) out of bounds (%lld)\n",
						fCurrentSection->name, index,
						fCurrentSection->stringsCount);
					return B_BAD_DATA;
				}

				_value.SetTo(fCurrentSection->strings[index]);
			} else if (encoding == B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE) {
				const char* string;
				status_t error = _ReadString(string);
				if (error != B_OK)
					return error;

				_value.SetTo(string);
			} else {
				fErrorOutput->PrintError("Error: Invalid %s section: invalid "
					"string encoding (%u)\n", fCurrentSection->name, encoding);
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
					fErrorOutput->PrintError("Error: Invalid %s section: "
						"invalid data reference\n", fCurrentSection->name);
					return B_BAD_DATA;
				}

				_value.SetToData(size, fHeapOffset + offset);
			} else if (encoding == B_HPKG_ATTRIBUTE_ENCODING_RAW_INLINE) {
				if (size > B_HPKG_MAX_INLINE_DATA_SIZE) {
					fErrorOutput->PrintError("Error: Invalid %s section: "
						"inline data too long\n", fCurrentSection->name);
					return B_BAD_DATA;
				}

				const void* buffer;
				error = _GetTOCBuffer(size, buffer);
				if (error != B_OK)
					return error;
				_value.SetToData(size, buffer);
			} else {
				fErrorOutput->PrintError("Error: Invalid %s section: invalid "
					"raw encoding (%u)\n", fCurrentSection->name, encoding);
				return B_BAD_DATA;
			}

			return B_OK;
		}

		default:
			fErrorOutput->PrintError("Error: Invalid %s section: invalid "
				"value type: %d\n", fCurrentSection->name, type);
			return B_BAD_DATA;
	}
}


status_t
PackageReaderImpl::_ReadUnsignedLEB128(uint64& _value)
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
PackageReaderImpl::_ReadString(const char*& _string, size_t* _stringLength)
{
	const char* string
		= (const char*)fCurrentSection->data + fCurrentSection->currentOffset;
	size_t stringLength = strnlen(string,
		fCurrentSection->uncompressedLength - fCurrentSection->currentOffset);

	if (stringLength
		== fCurrentSection->uncompressedLength
			- fCurrentSection->currentOffset) {
		fErrorOutput->PrintError(
			"_ReadString(): string extends beyond %s end\n",
			fCurrentSection->name);
		return B_BAD_DATA;
	}

	_string = string;
	if (_stringLength != NULL)
		*_stringLength = stringLength;

	fCurrentSection->currentOffset += stringLength + 1;
	return B_OK;
}


status_t
PackageReaderImpl::_GetTOCBuffer(size_t size, const void*& _buffer)
{
	if (size > fTOCSection.uncompressedLength - fTOCSection.currentOffset) {
		fErrorOutput->PrintError("_GetTOCBuffer(%lu): read beyond TOC end\n",
			size);
		return B_BAD_DATA;
	}

	_buffer = fTOCSection.data + fTOCSection.currentOffset;
	fTOCSection.currentOffset += size;
	return B_OK;
}


status_t
PackageReaderImpl::_ReadSectionBuffer(void* buffer, size_t size)
{
	if (size > fCurrentSection->uncompressedLength
			- fCurrentSection->currentOffset) {
		fErrorOutput->PrintError("_ReadBuffer(%lu): read beyond %s end\n",
			size, fCurrentSection->name);
		return B_BAD_DATA;
	}

	memcpy(buffer, fCurrentSection->data + fCurrentSection->currentOffset,
		size);
	fCurrentSection->currentOffset += size;
	return B_OK;
}


status_t
PackageReaderImpl::_ReadBuffer(off_t offset, void* buffer, size_t size)
{
	ssize_t bytesRead = pread(fFD, buffer, size, offset);
	if (bytesRead < 0) {
		fErrorOutput->PrintError("_ReadBuffer(%p, %lu) failed to read data: "
			"%s\n", buffer, size, strerror(errno));
		return errno;
	}
	if ((size_t)bytesRead != size) {
		fErrorOutput->PrintError("_ReadBuffer(%p, %lu) failed to read all "
			"data\n", buffer, size);
		return B_ERROR;
	}

	return B_OK;
}


status_t
PackageReaderImpl::_ReadCompressedBuffer(const SectionInfo& section)
{
	uint32 compressedSize = section.compressedLength;
	uint64 offset = section.offset;

	switch (section.compression) {
		case B_HPKG_COMPRESSION_NONE:
			return _ReadBuffer(offset, section.data, compressedSize);

		case B_HPKG_COMPRESSION_ZLIB:
		{
			// init the decompressor
			BBufferDataOutput bufferOutput(section.data,
				section.uncompressedLength);
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
			if (bufferOutput.BytesWritten() != section.uncompressedLength) {
				fErrorOutput->PrintError("Error: Missing bytes in uncompressed "
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
PackageReaderImpl::_GetStandardIndex(const AttributeType* type)
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


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
