/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
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

#include <FdIO.h>

#include <package/hpkg/HPKGDefsPrivate.h>

#include <package/hpkg/PackageData.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


//#define TRACE(format...)	printf(format)
#define TRACE(format...)	do {} while (false)


// maximum TOC size we support reading
static const size_t kMaxTOCSize					= 64 * 1024 * 1024;

// maximum package attributes size we support reading
static const size_t kMaxPackageAttributesSize	= 1 * 1024 * 1024;


static status_t
set_package_data_from_attribute_value(const BPackageAttributeValue& value,
	BPackageData& data)
{
	if (value.encoding == B_HPKG_ATTRIBUTE_ENCODING_RAW_INLINE)
		data.SetData(value.data.size, value.data.raw);
	else
		data.SetData(value.data.size, value.data.offset);
	return B_OK;
}


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
				return set_package_data_from_attribute_value(value,
					fAttribute.Data());

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
				return set_package_data_from_attribute_value(value,
					fEntry.Data());

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
	inherited("package", errorOutput),
	fTOCSection("TOC")
{
}


PackageReaderImpl::~PackageReaderImpl()
{
}


status_t
PackageReaderImpl::Init(const char* fileName, uint32 flags)
{
	// open file
	int fd = open(fileName, O_RDONLY);
	if (fd < 0) {
		ErrorOutput()->PrintError("Error: Failed to open package file \"%s\": "
			"%s\n", fileName, strerror(errno));
		return errno;
	}

	return Init(fd, true, flags);
}


status_t
PackageReaderImpl::Init(int fd, bool keepFD, uint32 flags)
{
	BFdIO* file = new(std::nothrow) BFdIO(fd, keepFD);
	if (file == NULL) {
		if (keepFD && fd >= 0)
			close(fd);
		return B_NO_MEMORY;
	}

	return Init(file, true, flags);
}


status_t
PackageReaderImpl::Init(BPositionIO* file, bool keepFile, uint32 flags,
	hpkg_header* _header)
{
	hpkg_header header;
	status_t error = inherited::Init<hpkg_header, B_HPKG_MAGIC, B_HPKG_VERSION,
		B_HPKG_MINOR_VERSION>(file, keepFile, header, flags);
	if (error != B_OK)
		return error;
	fHeapSize = UncompressedHeapSize();

	// init package attributes section
	error = InitSection(fPackageAttributesSection, fHeapSize,
		B_BENDIAN_TO_HOST_INT32(header.attributes_length),
		kMaxPackageAttributesSize,
		B_BENDIAN_TO_HOST_INT32(header.attributes_strings_length),
		B_BENDIAN_TO_HOST_INT32(header.attributes_strings_count));
	if (error != B_OK)
		return error;

	// init TOC section
	error = InitSection(fTOCSection, fPackageAttributesSection.offset,
		B_BENDIAN_TO_HOST_INT64(header.toc_length), kMaxTOCSize,
		B_BENDIAN_TO_HOST_INT64(header.toc_strings_length),
		B_BENDIAN_TO_HOST_INT64(header.toc_strings_count));
	if (error != B_OK)
		return error;

	if (_header != NULL)
		*_header = header;

	return B_OK;
}


status_t
PackageReaderImpl::ParseContent(BPackageContentHandler* contentHandler)
{
	status_t error = _PrepareSections();
	if (error != B_OK)
		return error;

	AttributeHandlerContext context(ErrorOutput(), contentHandler,
		B_HPKG_SECTION_PACKAGE_ATTRIBUTES,
		MinorFormatVersion() > B_HPKG_MINOR_VERSION);
	RootAttributeHandler rootAttributeHandler;

	error = ParsePackageAttributesSection(&context, &rootAttributeHandler);

	if (error == B_OK) {
		context.section = B_HPKG_SECTION_PACKAGE_TOC;
		error = _ParseTOC(&context, &rootAttributeHandler);
	}

	return error;
}


status_t
PackageReaderImpl::ParseContent(BLowLevelPackageContentHandler* contentHandler)
{
	status_t error = _PrepareSections();
	if (error != B_OK)
		return error;

	AttributeHandlerContext context(ErrorOutput(), contentHandler,
		B_HPKG_SECTION_PACKAGE_ATTRIBUTES,
		MinorFormatVersion() > B_HPKG_MINOR_VERSION);
	LowLevelAttributeHandler rootAttributeHandler;

	error = ParsePackageAttributesSection(&context, &rootAttributeHandler);

	if (error == B_OK) {
		context.section = B_HPKG_SECTION_PACKAGE_TOC;
		error = _ParseTOC(&context, &rootAttributeHandler);
	}

	return error;
}


status_t
PackageReaderImpl::_PrepareSections()
{
	status_t error = PrepareSection(fTOCSection);
	if (error != B_OK)
		return error;

	error = PrepareSection(fPackageAttributesSection);
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
PackageReaderImpl::_ParseTOC(AttributeHandlerContext* context,
	AttributeHandler* rootAttributeHandler)
{
	// parse the TOC
	fTOCSection.currentOffset = fTOCSection.stringsLength;
	SetCurrentSection(&fTOCSection);

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

				_value.SetToData(size, offset);
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
