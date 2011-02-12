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

static const size_t kScratchBufferSize			= 64 * 1024;


// #pragma mark - AttributeHandler


struct PackageReaderImpl::AttributeHandlerContext {
	BErrorOutput*	errorOutput;
	union {
		BPackageContentHandler*			packageContentHandler;
		BLowLevelPackageContentHandler*	lowLevelPackageContentHandler;
	};
	bool			hasLowLevelHandler;

	uint64			heapOffset;
	uint64			heapSize;


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

	virtual status_t HandleAttribute(AttributeHandlerContext* context,
		uint8 id, const AttributeValue& value, AttributeHandler** _handler)
	{
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


struct PackageReaderImpl::PackageVersionAttributeHandler : AttributeHandler {
	PackageVersionAttributeHandler(BPackageInfoAttributeValue& packageInfoValue,
		BPackageVersionData& versionData, bool notify)
		:
		fPackageInfoValue(packageInfoValue),
		fPackageVersionData(versionData),
		fNotify(notify)
	{
	}

	virtual status_t HandleAttribute(AttributeHandlerContext* context,
		uint8 id, const AttributeValue& value, AttributeHandler** _handler)
	{
		switch (id) {
			case B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MINOR:
				fPackageVersionData.minor = value.string;
				break;

			case B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MICRO:
				fPackageVersionData.micro = value.string;
				break;

			case B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_RELEASE:
				fPackageVersionData.release = value.unsignedInt;
				break;

			default:
				context->errorOutput->PrintError("Error: Invalid package "
					"attribute section: unexpected package attribute id %d "
					"encountered when parsing package version\n", id);
				return B_BAD_DATA;
		}

		return B_OK;
	}

	virtual status_t Delete(AttributeHandlerContext* context)
	{
		status_t error = B_OK;
		if (fNotify) {
			fPackageInfoValue.attributeID = B_PACKAGE_INFO_VERSION;
			error = context->packageContentHandler->HandlePackageAttribute(
				fPackageInfoValue);
			fPackageInfoValue.Clear();
		}

		delete this;
		return error;
	}

private:
	BPackageInfoAttributeValue&	fPackageInfoValue;
	BPackageVersionData&		fPackageVersionData;
	bool						fNotify;
};


struct PackageReaderImpl::PackageResolvableAttributeHandler : AttributeHandler {
	PackageResolvableAttributeHandler(
		BPackageInfoAttributeValue& packageInfoValue)
		:
		fPackageInfoValue(packageInfoValue)
	{
	}

	virtual status_t HandleAttribute(AttributeHandlerContext* context,
		uint8 id, const AttributeValue& value, AttributeHandler** _handler)
	{
		switch (id) {
			case B_HPKG_ATTRIBUTE_ID_PACKAGE_PROVIDES_TYPE:
				fPackageInfoValue.resolvable.type
					= (BPackageResolvableType)value.unsignedInt;
				break;

			case B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MAJOR:
				fPackageInfoValue.resolvable.haveVersion = true;
				fPackageInfoValue.resolvable.version.major = value.string;
				if (_handler != NULL) {
					*_handler
						= new(std::nothrow) PackageVersionAttributeHandler(
							fPackageInfoValue,
							fPackageInfoValue.resolvable.version, false);
					if (*_handler == NULL)
						return B_NO_MEMORY;
				}
				break;

			default:
				context->errorOutput->PrintError("Error: Invalid package "
					"attribute section: unexpected package attribute id %d "
					"encountered when parsing package resolvable\n", id);
				return B_BAD_DATA;
		}

		return B_OK;
	}

	virtual status_t Delete(AttributeHandlerContext* context)
	{
		status_t error = context->packageContentHandler->HandlePackageAttribute(
			fPackageInfoValue);
		fPackageInfoValue.Clear();

		delete this;
		return error;
	}

private:
	BPackageInfoAttributeValue&	fPackageInfoValue;
};


struct PackageReaderImpl::PackageResolvableExpressionAttributeHandler
	: AttributeHandler {
	PackageResolvableExpressionAttributeHandler(
		BPackageInfoAttributeValue& packageInfoValue)
		:
		fPackageInfoValue(packageInfoValue)
	{
	}

	virtual status_t HandleAttribute(AttributeHandlerContext* context,
		uint8 id, const AttributeValue& value, AttributeHandler** _handler)
	{
		switch (id) {
			case B_HPKG_ATTRIBUTE_ID_PACKAGE_RESOLVABLE_OPERATOR:
				if (value.unsignedInt >= B_PACKAGE_RESOLVABLE_OP_ENUM_COUNT) {
					context->errorOutput->PrintError(
						"Error: Invalid package attribute section: invalid "
						"package resolvable operator %lld encountered\n",
						value.unsignedInt);
					return B_BAD_DATA;
				}
				fPackageInfoValue.resolvableExpression.op
					= (BPackageResolvableOperator)value.unsignedInt;
				break;

			case B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MAJOR:
				fPackageInfoValue.resolvableExpression.haveOpAndVersion = true;
				fPackageInfoValue.resolvableExpression.version.major
					= value.string;
				if (_handler != NULL) {
					*_handler
						= new(std::nothrow) PackageVersionAttributeHandler(
							fPackageInfoValue,
							fPackageInfoValue.resolvableExpression.version,
							false);
					if (*_handler == NULL)
						return B_NO_MEMORY;
				}
				return B_OK;

			default:
				context->errorOutput->PrintError("Error: Invalid package "
					"attribute section: unexpected package attribute id %d "
					"encountered when parsing package resolvable-expression\n",
					id);
				return B_BAD_DATA;
		}

		return B_OK;
	}

	virtual status_t Delete(AttributeHandlerContext* context)
	{
		status_t error = context->packageContentHandler->HandlePackageAttribute(
			fPackageInfoValue);
		fPackageInfoValue.Clear();

		delete this;
		return error;
	}

private:
	BPackageInfoAttributeValue&	fPackageInfoValue;
};


struct PackageReaderImpl::PackageAttributeHandler : AttributeHandler {
	virtual status_t HandleAttribute(AttributeHandlerContext* context,
		uint8 id, const AttributeValue& value, AttributeHandler** _handler)
	{
		switch (id) {
			case B_HPKG_ATTRIBUTE_ID_PACKAGE_NAME:
				fPackageInfoValue.SetTo(B_PACKAGE_INFO_NAME, value.string);
				break;

			case B_HPKG_ATTRIBUTE_ID_PACKAGE_SUMMARY:
				fPackageInfoValue.SetTo(B_PACKAGE_INFO_SUMMARY, value.string);
				break;

			case B_HPKG_ATTRIBUTE_ID_PACKAGE_DESCRIPTION:
				fPackageInfoValue.SetTo(B_PACKAGE_INFO_DESCRIPTION,
					value.string);
				break;

			case B_HPKG_ATTRIBUTE_ID_PACKAGE_VENDOR:
				fPackageInfoValue.SetTo(B_PACKAGE_INFO_VENDOR, value.string);
				break;

			case B_HPKG_ATTRIBUTE_ID_PACKAGE_PACKAGER:
				fPackageInfoValue.SetTo(B_PACKAGE_INFO_PACKAGER, value.string);
				break;

			case B_HPKG_ATTRIBUTE_ID_PACKAGE_FLAGS:
				fPackageInfoValue.SetTo(B_PACKAGE_INFO_FLAGS,
					(uint32)value.unsignedInt);
				break;

			case B_HPKG_ATTRIBUTE_ID_PACKAGE_ARCHITECTURE:
				if (value.unsignedInt
						>= B_PACKAGE_ARCHITECTURE_ENUM_COUNT) {
					context->errorOutput->PrintError(
						"Error: Invalid package attribute section: "
						"Invalid package architecture %lld encountered\n",
						value.unsignedInt);
					return B_BAD_DATA;
				}
				fPackageInfoValue.SetTo(B_PACKAGE_INFO_ARCHITECTURE,
					(uint8)value.unsignedInt);
				break;

			case B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MAJOR:
				fPackageInfoValue.attributeID = B_PACKAGE_INFO_VERSION;
				fPackageInfoValue.version.major = value.string;
				if (_handler != NULL) {
					*_handler
						= new(std::nothrow) PackageVersionAttributeHandler(
							fPackageInfoValue, fPackageInfoValue.version, true);
					if (*_handler == NULL)
						return B_NO_MEMORY;
				}
				break;

			case B_HPKG_ATTRIBUTE_ID_PACKAGE_COPYRIGHT:
				fPackageInfoValue.SetTo(B_PACKAGE_INFO_COPYRIGHTS,
					value.string);
				break;

			case B_HPKG_ATTRIBUTE_ID_PACKAGE_LICENSE:
				fPackageInfoValue.SetTo(B_PACKAGE_INFO_LICENSES,
					value.string);
				break;

			case B_HPKG_ATTRIBUTE_ID_PACKAGE_PROVIDES:
				fPackageInfoValue.resolvable.name = value.string;
				fPackageInfoValue.attributeID = B_PACKAGE_INFO_PROVIDES;
				if (_handler != NULL) {
					*_handler
						= new(std::nothrow) PackageResolvableAttributeHandler(
							fPackageInfoValue);
					if (*_handler == NULL)
						return B_NO_MEMORY;
				}
				break;

			case B_HPKG_ATTRIBUTE_ID_PACKAGE_REQUIRES:
			case B_HPKG_ATTRIBUTE_ID_PACKAGE_SUPPLEMENTS:
			case B_HPKG_ATTRIBUTE_ID_PACKAGE_CONFLICTS:
			case B_HPKG_ATTRIBUTE_ID_PACKAGE_FRESHENS:
				fPackageInfoValue.resolvableExpression.name = value.string;
				switch (id) {
					case B_HPKG_ATTRIBUTE_ID_PACKAGE_REQUIRES:
						fPackageInfoValue.attributeID = B_PACKAGE_INFO_REQUIRES;
						break;

					case B_HPKG_ATTRIBUTE_ID_PACKAGE_SUPPLEMENTS:
						fPackageInfoValue.attributeID
							= B_PACKAGE_INFO_SUPPLEMENTS;
						break;

					case B_HPKG_ATTRIBUTE_ID_PACKAGE_CONFLICTS:
						fPackageInfoValue.attributeID
							= B_PACKAGE_INFO_CONFLICTS;
						break;

					case B_HPKG_ATTRIBUTE_ID_PACKAGE_FRESHENS:
						fPackageInfoValue.attributeID = B_PACKAGE_INFO_FRESHENS;
						break;
				}
				if (_handler != NULL) {
					*_handler = new(std::nothrow)
						PackageResolvableExpressionAttributeHandler(
							fPackageInfoValue);
					if (*_handler == NULL)
						return B_NO_MEMORY;
				}
				break;

			case B_HPKG_ATTRIBUTE_ID_PACKAGE_REPLACES:
				fPackageInfoValue.SetTo(B_PACKAGE_INFO_REPLACES, value.string);
				break;

			default:
				context->errorOutput->PrintError(
					"Error: Invalid package attribute section: unexpected "
					"package attribute id %d encountered\n", id);
				return B_BAD_DATA;
		}

		// notify unless the current attribute has children, in which case
		// the child-handler will notify when it's done
		if (_handler == NULL) {
			status_t error = context->packageContentHandler
				->HandlePackageAttribute(fPackageInfoValue);
			fPackageInfoValue.Clear();
			if (error != B_OK)
				return error;
		}

		return B_OK;
	}

private:
	BPackageInfoAttributeValue	fPackageInfoValue;
};


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


struct PackageReaderImpl::LowLevelAttributeHandler : AttributeHandler {
	LowLevelAttributeHandler()
		:
		fToken(NULL),
		fID(B_HPKG_ATTRIBUTE_ID_ENUM_COUNT)
	{
	}

	LowLevelAttributeHandler(uint8 id, const BPackageAttributeValue& value,
		void* token)
		:
		fToken(token),
		fID(id),
		fValue(value)
	{
	}

	virtual status_t HandleAttribute(AttributeHandlerContext* context,
		uint8 id, const AttributeValue& value, AttributeHandler** _handler)
	{
		// notify the content handler
		void* token;
		status_t error = context->lowLevelPackageContentHandler
			->HandleAttribute((BHPKGAttributeID)id, value, fToken, token);
		if (error != B_OK)
			return error;

		// create a subhandler for the attribute, if it has children
		if (_handler != NULL) {
			*_handler = new(std::nothrow) LowLevelAttributeHandler(id, value,
				token);
			if (*_handler == NULL) {
				context->lowLevelPackageContentHandler->HandleAttributeDone(
					(BHPKGAttributeID)id, value, token);
				return B_NO_MEMORY;
			}
			return B_OK;
		}

		// no children -- just call the done hook
		return context->lowLevelPackageContentHandler->HandleAttributeDone(
			(BHPKGAttributeID)id, value, token);
	}

	virtual status_t Delete(AttributeHandlerContext* context)
	{
		status_t error = B_OK;
		if (fID != B_HPKG_ATTRIBUTE_ID_ENUM_COUNT) {
			error = context->lowLevelPackageContentHandler->HandleAttributeDone(
				(BHPKGAttributeID)fID, fValue, fToken);
		}

		delete this;
		return error;
	}

private:

	void*			fToken;
	uint8			fID;
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
	fScratchBuffer(NULL),
	fScratchBufferSize(0)
{
}


PackageReaderImpl::~PackageReaderImpl()
{
	if (fOwnsFD && fFD >= 0)
		close(fFD);

	delete[] fScratchBuffer;
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
	fTOCSection.stringsLength
		= B_BENDIAN_TO_HOST_INT64(header.toc_strings_length);
	fTOCSection.stringsCount
		= B_BENDIAN_TO_HOST_INT64(header.toc_strings_count);

	if (fTOCSection.stringsLength > fTOCSection.uncompressedLength
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

	// strings
	error = _ParseStrings();
	if (error != B_OK)
		return error;
	fCurrentSection->currentOffset += fCurrentSection->stringsLength;

	// parse strings from package attributes section
	fCurrentSection = &fPackageAttributesSection;
	fCurrentSection->currentOffset = 0;

	// strings
	error = _ParseStrings();
	if (error != B_OK)
		return error;
	fCurrentSection->currentOffset += fCurrentSection->stringsLength;

	fCurrentSection = NULL;

	return B_OK;
}


status_t
PackageReaderImpl::ParseContent(BPackageContentHandler* contentHandler)
{
	AttributeHandlerContext context(fErrorOutput, contentHandler);
	RootAttributeHandler rootAttributeHandler;
	return _ParseContent(&context, &rootAttributeHandler);
}


status_t
PackageReaderImpl::ParseContent(BLowLevelPackageContentHandler* contentHandler)
{
	AttributeHandlerContext context(fErrorOutput, contentHandler);
	LowLevelAttributeHandler rootAttributeHandler;
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
	fCurrentSection = &fTOCSection;
	fTOCSection.currentOffset = fTOCSection.stringsLength;

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

	// parse package attributes
	fCurrentSection = &fPackageAttributesSection;
	fAttributeHandlerStack->Add(rootAttributeHandler);
		// re-init the attribute handler stack
	if (error == B_OK)
		error = _ParseAttributeTree(context);
	if (error == B_OK) {
		if (fPackageAttributesSection.currentOffset
				< fPackageAttributesSection.uncompressedLength) {
			fErrorOutput->PrintError("Error: %llu excess byte(s) in package "
				"attributes section\n",
				fPackageAttributesSection.uncompressedLength
					- fPackageAttributesSection.currentOffset);
			error = B_BAD_DATA;
		}
	}

	fCurrentSection = NULL;

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
		uint8 id;
		AttributeValue value;
		bool hasChildren;
		uint64 tag;

		status_t error = _ReadAttribute(id, value, &hasChildren, &tag);
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

		AttributeHandler* childHandler = NULL;
		error = _CurrentAttributeHandler()->HandleAttribute(context, id, value,
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

			childHandler->SetLevel(++level);
			_PushAttributeHandler(childHandler);
		}
	}
}


status_t
PackageReaderImpl::_ReadAttribute(uint8& _id, AttributeValue& _value,
	bool* _hasChildren, uint64* _tag)
{
	uint64 tag;
	status_t error = _ReadUnsignedLEB128(tag);
	if (error != B_OK)
		return error;

	if (tag != 0) {
		// get the type
		uint16 type = HPKG_ATTRIBUTE_TAG_TYPE(tag);
		if (type >= B_HPKG_ATTRIBUTE_TYPE_ENUM_COUNT) {
			fErrorOutput->PrintError("Error: Invalid %s section: attribute "
				"type %d not supported!\n", fCurrentSection->name, type);
			return B_BAD_DATA;
		}

		// get the value
		error = _ReadAttributeValue(type, HPKG_ATTRIBUTE_TAG_ENCODING(tag),
			_value);
		if (error != B_OK)
			return error;

		_id = HPKG_ATTRIBUTE_TAG_ID(tag);
		if (_id >= B_HPKG_ATTRIBUTE_ID_ENUM_COUNT) {
			fErrorOutput->PrintError("Error: Invalid %s section: "
				"attribute id %d not supported!\n", fCurrentSection->name, _id);
			return B_BAD_DATA;
		}
	}

	if (_hasChildren != NULL)
		*_hasChildren = HPKG_ATTRIBUTE_TAG_HAS_CHILDREN(tag);
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


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
