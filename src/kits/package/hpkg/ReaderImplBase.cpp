/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/ReaderImplBase.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <new>

#include <ByteOrder.h>

#include <package/hpkg/HPKGDefsPrivate.h>

#include <package/hpkg/DataOutput.h>
#include <package/hpkg/ErrorOutput.h>
#include <package/hpkg/ZlibDecompressor.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


static const size_t kScratchBufferSize = 64 * 1024;


// #pragma mark - AttributeHandlerContext


ReaderImplBase::AttributeHandlerContext::AttributeHandlerContext(
	BErrorOutput* errorOutput, BPackageContentHandler* packageContentHandler)
	:
	errorOutput(errorOutput),
	packageContentHandler(packageContentHandler),
	hasLowLevelHandler(false)
{
}


ReaderImplBase::AttributeHandlerContext::AttributeHandlerContext(
	BErrorOutput* errorOutput, BLowLevelPackageContentHandler* lowLevelHandler)
	:
	errorOutput(errorOutput),
	lowLevelHandler(lowLevelHandler),
	hasLowLevelHandler(true)
{
}


void
ReaderImplBase::AttributeHandlerContext::ErrorOccurred()
{
	if (hasLowLevelHandler)
		lowLevelHandler->HandleErrorOccurred();
	else
		packageContentHandler->HandleErrorOccurred();
}


// #pragma mark - AttributeHandler


ReaderImplBase::AttributeHandler::~AttributeHandler()
{
}


void
ReaderImplBase::AttributeHandler::SetLevel(int level)
{
	fLevel = level;
}


status_t
ReaderImplBase::AttributeHandler::HandleAttribute(
	AttributeHandlerContext* context, uint8 id, const AttributeValue& value,
	AttributeHandler** _handler)
{
	return B_OK;
}


status_t
ReaderImplBase::AttributeHandler::Delete(AttributeHandlerContext* context)
{
	delete this;
	return B_OK;
}


// #pragma mark - PackageVersionAttributeHandler


ReaderImplBase::PackageVersionAttributeHandler::PackageVersionAttributeHandler(
	BPackageInfoAttributeValue& packageInfoValue,
	BPackageVersionData& versionData, bool notify)
	:
	fPackageInfoValue(packageInfoValue),
	fPackageVersionData(versionData),
	fNotify(notify)
{
}


status_t
ReaderImplBase::PackageVersionAttributeHandler::HandleAttribute(
	AttributeHandlerContext* context, uint8 id, const AttributeValue& value,
	AttributeHandler** _handler)
{
	switch (id) {
		case B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MINOR:
			fPackageVersionData.minor = value.string;
			break;

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MICRO:
			fPackageVersionData.micro = value.string;
			break;

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_PRE_RELEASE:
			fPackageVersionData.preRelease = value.string;
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


status_t
ReaderImplBase::PackageVersionAttributeHandler::Delete(
	AttributeHandlerContext* context)
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


// #pragma mark - PackageResolvableAttributeHandler


ReaderImplBase::PackageResolvableAttributeHandler
	::PackageResolvableAttributeHandler(
		BPackageInfoAttributeValue& packageInfoValue)
	:
	fPackageInfoValue(packageInfoValue)
{
}


status_t
ReaderImplBase::PackageResolvableAttributeHandler::HandleAttribute(
	AttributeHandlerContext* context, uint8 id, const AttributeValue& value,
	AttributeHandler** _handler)
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

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_PROVIDES_COMPATIBLE:
			fPackageInfoValue.resolvable.haveCompatibleVersion = true;
			fPackageInfoValue.resolvable.compatibleVersion.major = value.string;
			if (_handler != NULL) {
				*_handler
					= new(std::nothrow) PackageVersionAttributeHandler(
						fPackageInfoValue,
						fPackageInfoValue.resolvable.compatibleVersion, false);
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


status_t
ReaderImplBase::PackageResolvableAttributeHandler::Delete(
	AttributeHandlerContext* context)
{
	status_t error = context->packageContentHandler->HandlePackageAttribute(
		fPackageInfoValue);
	fPackageInfoValue.Clear();

	delete this;
	return error;
}


// #pragma mark - PackageResolvableExpressionAttributeHandler


ReaderImplBase::PackageResolvableExpressionAttributeHandler
	::PackageResolvableExpressionAttributeHandler(
		BPackageInfoAttributeValue& packageInfoValue)
	:
	fPackageInfoValue(packageInfoValue)
{
}


status_t
ReaderImplBase::PackageResolvableExpressionAttributeHandler::HandleAttribute(
	AttributeHandlerContext* context, uint8 id, const AttributeValue& value,
	AttributeHandler** _handler)
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


status_t
ReaderImplBase::PackageResolvableExpressionAttributeHandler::Delete(
	AttributeHandlerContext* context)
{
	status_t error = context->packageContentHandler->HandlePackageAttribute(
		fPackageInfoValue);
	fPackageInfoValue.Clear();

	delete this;
	return error;
}


// #pragma mark - PackageAttributeHandler


status_t
ReaderImplBase::PackageAttributeHandler::HandleAttribute(
	AttributeHandlerContext* context, uint8 id, const AttributeValue& value,
	AttributeHandler** _handler)
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

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_URL:
			fPackageInfoValue.SetTo(B_PACKAGE_INFO_URLS, value.string);
			break;

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_SOURCE_URL:
			fPackageInfoValue.SetTo(B_PACKAGE_INFO_SOURCE_URLS, value.string);
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

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_CHECKSUM:
			fPackageInfoValue.SetTo(B_PACKAGE_INFO_CHECKSUM, value.string);
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


// #pragma mark - LowLevelAttributeHandler


ReaderImplBase::LowLevelAttributeHandler::LowLevelAttributeHandler()
	:
	fToken(NULL),
	fID(B_HPKG_ATTRIBUTE_ID_ENUM_COUNT)
{
}


ReaderImplBase::LowLevelAttributeHandler::LowLevelAttributeHandler(uint8 id,
	const BPackageAttributeValue& value, void* token)
	:
	fToken(token),
	fID(id),
	fValue(value)
{
}


status_t
ReaderImplBase::LowLevelAttributeHandler::HandleAttribute(
	AttributeHandlerContext* context, uint8 id, const AttributeValue& value,
	AttributeHandler** _handler)
{
	// notify the content handler
	void* token;
	status_t error = context->lowLevelHandler->HandleAttribute(
		(BHPKGAttributeID)id, value, fToken, token);
	if (error != B_OK)
		return error;

	// create a subhandler for the attribute, if it has children
	if (_handler != NULL) {
		*_handler = new(std::nothrow) LowLevelAttributeHandler(id, value,
			token);
		if (*_handler == NULL) {
			context->lowLevelHandler->HandleAttributeDone((BHPKGAttributeID)id,
				value, token);
			return B_NO_MEMORY;
		}
		return B_OK;
	}

	// no children -- just call the done hook
	return context->lowLevelHandler->HandleAttributeDone((BHPKGAttributeID)id,
		value, token);
}


status_t
ReaderImplBase::LowLevelAttributeHandler::Delete(
	AttributeHandlerContext* context)
{
	status_t error = B_OK;
	if (fID != B_HPKG_ATTRIBUTE_ID_ENUM_COUNT) {
		error = context->lowLevelHandler->HandleAttributeDone(
			(BHPKGAttributeID)fID, fValue, fToken);
	}

	delete this;
	return error;
}


// #pragma mark - ReaderImplBase


ReaderImplBase::ReaderImplBase(BErrorOutput* errorOutput)
	:
	fPackageAttributesSection("package attributes"),
	fErrorOutput(errorOutput),
	fFD(-1),
	fOwnsFD(false),
	fCurrentSection(NULL),
	fScratchBuffer(NULL),
	fScratchBufferSize(0)
{
}


ReaderImplBase::~ReaderImplBase()
{
	if (fOwnsFD && fFD >= 0)
		close(fFD);

	delete[] fScratchBuffer;
}


status_t
ReaderImplBase::Init(int fd, bool keepFD)
{
	fFD = fd;
	fOwnsFD = keepFD;

	// allocate a scratch buffer
	fScratchBuffer = new(std::nothrow) uint8[kScratchBufferSize];
	if (fScratchBuffer == NULL) {
		fErrorOutput->PrintError("Error: Out of memory!\n");
		return B_NO_MEMORY;
	}
	fScratchBufferSize = kScratchBufferSize;

	return B_OK;
}


const char*
ReaderImplBase::CheckCompression(const SectionInfo& section) const
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
ReaderImplBase::ParseStrings()
{
	// allocate table, if there are any strings
	if (fCurrentSection->stringsCount == 0) {
		fCurrentSection->currentOffset += fCurrentSection->stringsLength;
		return B_OK;
	}

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

			fCurrentSection->currentOffset += fCurrentSection->stringsLength;

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
ReaderImplBase::ParsePackageAttributesSection(
	AttributeHandlerContext* context, AttributeHandler* rootAttributeHandler)
{
	// parse package attributes
	SetCurrentSection(&fPackageAttributesSection);

	// init the attribute handler stack
	rootAttributeHandler->SetLevel(0);
	ClearAttributeHandlerStack();
	PushAttributeHandler(rootAttributeHandler);

	status_t error = ParseAttributeTree(context);
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

	SetCurrentSection(NULL);

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
ReaderImplBase::ParseAttributeTree(AttributeHandlerContext* context)
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
			AttributeHandler* handler = PopAttributeHandler();
			if (level-- == 0)
				return B_OK;

			error = handler->Delete(context);
			if (error != B_OK)
				return error;

			continue;
		}

		AttributeHandler* childHandler = NULL;
		error = CurrentAttributeHandler()->HandleAttribute(context, id, value,
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
			PushAttributeHandler(childHandler);
		}
	}
}


status_t
ReaderImplBase::_ReadAttribute(uint8& _id, AttributeValue& _value,
	bool* _hasChildren, uint64* _tag)
{
	uint64 tag;
	status_t error = ReadUnsignedLEB128(tag);
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
		error = ReadAttributeValue(type, HPKG_ATTRIBUTE_TAG_ENCODING(tag),
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
ReaderImplBase::ReadAttributeValue(uint8 type, uint8 encoding,
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
				status_t error = ReadUnsignedLEB128(index);
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

		default:
			fErrorOutput->PrintError("Error: Invalid %s section: invalid "
				"value type: %d\n", fCurrentSection->name, type);
			return B_BAD_DATA;
	}
}


status_t
ReaderImplBase::ReadUnsignedLEB128(uint64& _value)
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
ReaderImplBase::_ReadString(const char*& _string, size_t* _stringLength)
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
ReaderImplBase::_ReadSectionBuffer(void* buffer, size_t size)
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
ReaderImplBase::ReadBuffer(off_t offset, void* buffer, size_t size)
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
ReaderImplBase::ReadCompressedBuffer(const SectionInfo& section)
{
	uint32 compressedSize = section.compressedLength;
	uint64 offset = section.offset;

	switch (section.compression) {
		case B_HPKG_COMPRESSION_NONE:
			return ReadBuffer(offset, section.data, compressedSize);

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
				size_t toRead = std::min((size_t)compressedSize,
					fScratchBufferSize);
				error = ReadBuffer(offset, fScratchBuffer, toRead);
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
