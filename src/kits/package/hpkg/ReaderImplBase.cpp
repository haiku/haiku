/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/ReaderImplBase.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <new>

#include <ByteOrder.h>
#include <DataIO.h>

#include <ZlibCompressionAlgorithm.h>
#ifdef ZSTD_ENABLED
#include <ZstdCompressionAlgorithm.h>
#endif

#include <package/hpkg/HPKGDefsPrivate.h>
#include <package/hpkg/PackageFileHeapReader.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


static const uint16 kAttributeTypes[B_HPKG_ATTRIBUTE_ID_ENUM_COUNT] = {
	#define B_DEFINE_HPKG_ATTRIBUTE(id, type, name, constant)	\
		B_HPKG_ATTRIBUTE_TYPE_##type,
	#include <package/hpkg/PackageAttributes.h>
	#undef B_DEFINE_HPKG_ATTRIBUTE
};

// #pragma mark - AttributeHandlerContext


ReaderImplBase::AttributeHandlerContext::AttributeHandlerContext(
	BErrorOutput* errorOutput, BPackageContentHandler* packageContentHandler,
	BHPKGPackageSectionID section, bool ignoreUnknownAttributes)
	:
	errorOutput(errorOutput),
	packageContentHandler(packageContentHandler),
	hasLowLevelHandler(false),
	ignoreUnknownAttributes(ignoreUnknownAttributes),
	section(section)
{
}


ReaderImplBase::AttributeHandlerContext::AttributeHandlerContext(
	BErrorOutput* errorOutput, BLowLevelPackageContentHandler* lowLevelHandler,
	BHPKGPackageSectionID section, bool ignoreUnknownAttributes)
	:
	errorOutput(errorOutput),
	lowLevelHandler(lowLevelHandler),
	hasLowLevelHandler(true),
	ignoreUnknownAttributes(ignoreUnknownAttributes),
	section(section)
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
ReaderImplBase::AttributeHandler::NotifyDone(
	AttributeHandlerContext* context)
{
	return B_OK;
}


status_t
ReaderImplBase::AttributeHandler::Delete(AttributeHandlerContext* context)
{
	delete this;
	return B_OK;
}


// #pragma mark - PackageInfoAttributeHandlerBase


ReaderImplBase::PackageInfoAttributeHandlerBase
	::PackageInfoAttributeHandlerBase(
		BPackageInfoAttributeValue& packageInfoValue)
	:
	fPackageInfoValue(packageInfoValue)
{
}


status_t
ReaderImplBase::PackageInfoAttributeHandlerBase::NotifyDone(
	AttributeHandlerContext* context)
{
	status_t error = context->packageContentHandler->HandlePackageAttribute(
		fPackageInfoValue);
	fPackageInfoValue.Clear();
	return error;
}


// #pragma mark - PackageVersionAttributeHandler


ReaderImplBase::PackageVersionAttributeHandler::PackageVersionAttributeHandler(
	BPackageInfoAttributeValue& packageInfoValue,
	BPackageVersionData& versionData, bool notify)
	:
	super(packageInfoValue),
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

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_REVISION:
			fPackageVersionData.revision = value.unsignedInt;
			break;

		default:
			if (context->ignoreUnknownAttributes)
				break;

			context->errorOutput->PrintError("Error: Invalid package "
				"attribute section: unexpected package attribute id %d "
				"encountered when parsing package version\n", id);
			return B_BAD_DATA;
	}

	return B_OK;
}


status_t
ReaderImplBase::PackageVersionAttributeHandler::NotifyDone(
	AttributeHandlerContext* context)
{
	if (!fNotify)
		return B_OK;

	fPackageInfoValue.attributeID = B_PACKAGE_INFO_VERSION;
	return super::NotifyDone(context);
}


// #pragma mark - PackageResolvableAttributeHandler


ReaderImplBase::PackageResolvableAttributeHandler
	::PackageResolvableAttributeHandler(
		BPackageInfoAttributeValue& packageInfoValue)
	:
	super(packageInfoValue)
{
}


status_t
ReaderImplBase::PackageResolvableAttributeHandler::HandleAttribute(
	AttributeHandlerContext* context, uint8 id, const AttributeValue& value,
	AttributeHandler** _handler)
{
	switch (id) {
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
			if (context->ignoreUnknownAttributes)
				break;

			context->errorOutput->PrintError("Error: Invalid package "
				"attribute section: unexpected package attribute id %d "
				"encountered when parsing package resolvable\n", id);
			return B_BAD_DATA;
	}

	return B_OK;
}


// #pragma mark - PackageResolvableExpressionAttributeHandler


ReaderImplBase::PackageResolvableExpressionAttributeHandler
	::PackageResolvableExpressionAttributeHandler(
		BPackageInfoAttributeValue& packageInfoValue)
	:
	super(packageInfoValue)
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
			if (context->ignoreUnknownAttributes)
				break;

			context->errorOutput->PrintError("Error: Invalid package "
				"attribute section: unexpected package attribute id %d "
				"encountered when parsing package resolvable-expression\n",
				id);
			return B_BAD_DATA;
	}

	return B_OK;
}


// #pragma mark - GlobalWritableFileInfoAttributeHandler


ReaderImplBase::GlobalWritableFileInfoAttributeHandler
	::GlobalWritableFileInfoAttributeHandler(
		BPackageInfoAttributeValue& packageInfoValue)
	:
	super(packageInfoValue)
{
}


status_t
ReaderImplBase::GlobalWritableFileInfoAttributeHandler::HandleAttribute(
	AttributeHandlerContext* context, uint8 id, const AttributeValue& value,
	AttributeHandler** _handler)
{
	switch (id) {
		case B_HPKG_ATTRIBUTE_ID_PACKAGE_WRITABLE_FILE_UPDATE_TYPE:
			if (value.unsignedInt >= B_WRITABLE_FILE_UPDATE_TYPE_ENUM_COUNT) {
				context->errorOutput->PrintError(
					"Error: Invalid package attribute section: invalid "
					"global settings file update type %" B_PRIu64
					" encountered\n", value.unsignedInt);
				return B_BAD_DATA;
			}
			fPackageInfoValue.globalWritableFileInfo.updateType
				= (BWritableFileUpdateType)value.unsignedInt;
			break;

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_IS_WRITABLE_DIRECTORY:
			fPackageInfoValue.globalWritableFileInfo.isDirectory
				= value.unsignedInt != 0;
			break;

		default:
			if (context->ignoreUnknownAttributes)
				break;

			context->errorOutput->PrintError("Error: Invalid package "
				"attribute section: unexpected package attribute id %d "
				"encountered when parsing global settings file info\n",
				id);
			return B_BAD_DATA;
	}

	return B_OK;
}


// #pragma mark - UserSettingsFileInfoAttributeHandler


ReaderImplBase::UserSettingsFileInfoAttributeHandler
	::UserSettingsFileInfoAttributeHandler(
		BPackageInfoAttributeValue& packageInfoValue)
	:
	super(packageInfoValue)
{
}


status_t
ReaderImplBase::UserSettingsFileInfoAttributeHandler::HandleAttribute(
	AttributeHandlerContext* context, uint8 id, const AttributeValue& value,
	AttributeHandler** _handler)
{
	switch (id) {
		case B_HPKG_ATTRIBUTE_ID_PACKAGE_SETTINGS_FILE_TEMPLATE:
			fPackageInfoValue.userSettingsFileInfo.templatePath = value.string;
			break;

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_IS_WRITABLE_DIRECTORY:
			fPackageInfoValue.userSettingsFileInfo.isDirectory
				= value.unsignedInt != 0;
			break;

		default:
			if (context->ignoreUnknownAttributes)
				break;

			context->errorOutput->PrintError("Error: Invalid package "
				"attribute section: unexpected package attribute id %d "
				"encountered when parsing user settings file info\n",
				id);
			return B_BAD_DATA;
	}

	return B_OK;
}


// #pragma mark - UserAttributeHandler


ReaderImplBase::UserAttributeHandler::UserAttributeHandler(
		BPackageInfoAttributeValue& packageInfoValue)
	:
	super(packageInfoValue),
	fGroups()
{
}


status_t
ReaderImplBase::UserAttributeHandler::HandleAttribute(
	AttributeHandlerContext* context, uint8 id, const AttributeValue& value,
	AttributeHandler** _handler)
{
	switch (id) {
		case B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_REAL_NAME:
			fPackageInfoValue.user.realName = value.string;
			break;

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_HOME:
			fPackageInfoValue.user.home = value.string;
			break;

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_SHELL:
			fPackageInfoValue.user.shell = value.string;
			break;

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_GROUP:
			if (!fGroups.Add(value.string))
				return B_NO_MEMORY;
			break;

		default:
			if (context->ignoreUnknownAttributes)
				break;

			context->errorOutput->PrintError("Error: Invalid package "
				"attribute section: unexpected package attribute id %d "
				"encountered when parsing user settings file info\n",
				id);
			return B_BAD_DATA;
	}

	return B_OK;
}


status_t
ReaderImplBase::UserAttributeHandler::NotifyDone(
	AttributeHandlerContext* context)
{
	if (!fGroups.IsEmpty()) {
		fPackageInfoValue.user.groups = fGroups.Elements();
		fPackageInfoValue.user.groupCount = fGroups.Count();
	}

	return super::NotifyDone(context);
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

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_BASE_PACKAGE:
			fPackageInfoValue.SetTo(B_PACKAGE_INFO_BASE_PACKAGE, value.string);
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

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_INSTALL_PATH:
			fPackageInfoValue.SetTo(B_PACKAGE_INFO_INSTALL_PATH, value.string);
			break;

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_GLOBAL_WRITABLE_FILE:
			fPackageInfoValue.globalWritableFileInfo.path = value.string;
			fPackageInfoValue.globalWritableFileInfo.updateType
				= B_WRITABLE_FILE_UPDATE_TYPE_ENUM_COUNT;
			fPackageInfoValue.attributeID
				= B_PACKAGE_INFO_GLOBAL_WRITABLE_FILES;
			if (_handler != NULL) {
				*_handler
					= new(std::nothrow) GlobalWritableFileInfoAttributeHandler(
						fPackageInfoValue);
				if (*_handler == NULL)
					return B_NO_MEMORY;
			}
			break;

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_SETTINGS_FILE:
			fPackageInfoValue.userSettingsFileInfo.path = value.string;
			fPackageInfoValue.attributeID
				= B_PACKAGE_INFO_USER_SETTINGS_FILES;
			if (_handler != NULL) {
				*_handler
					= new(std::nothrow) UserSettingsFileInfoAttributeHandler(
						fPackageInfoValue);
				if (*_handler == NULL)
					return B_NO_MEMORY;
			}
			break;

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_USER:
			fPackageInfoValue.user.name = value.string;
			fPackageInfoValue.attributeID = B_PACKAGE_INFO_USERS;
			if (_handler != NULL) {
				*_handler = new(std::nothrow) UserAttributeHandler(
					fPackageInfoValue);
				if (*_handler == NULL)
					return B_NO_MEMORY;
			}
			break;

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_GROUP:
			fPackageInfoValue.SetTo(B_PACKAGE_INFO_GROUPS, value.string);
			break;

		case B_HPKG_ATTRIBUTE_ID_PACKAGE_POST_INSTALL_SCRIPT:
			fPackageInfoValue.SetTo(B_PACKAGE_INFO_POST_INSTALL_SCRIPTS,
				value.string);
			break;

		default:
			if (context->ignoreUnknownAttributes)
				break;

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
	fParentToken(NULL),
	fToken(NULL),
	fID(B_HPKG_ATTRIBUTE_ID_ENUM_COUNT)
{
}


ReaderImplBase::LowLevelAttributeHandler::LowLevelAttributeHandler(uint8 id,
	const BPackageAttributeValue& value, void* parentToken, void* token)
	:
	fParentToken(NULL),
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
			fToken, token);
		if (*_handler == NULL) {
			context->lowLevelHandler->HandleAttributeDone((BHPKGAttributeID)id,
				value, fToken, token);
			return B_NO_MEMORY;
		}
		return B_OK;
	}

	// no children -- just call the done hook
	return context->lowLevelHandler->HandleAttributeDone((BHPKGAttributeID)id,
		value, fToken, token);
}


status_t
ReaderImplBase::LowLevelAttributeHandler::NotifyDone(
	AttributeHandlerContext* context)
{
	if (fID != B_HPKG_ATTRIBUTE_ID_ENUM_COUNT) {
		status_t error = context->lowLevelHandler->HandleAttributeDone(
			(BHPKGAttributeID)fID, fValue, fParentToken, fToken);
		if (error != B_OK)
			return error;
	}
	return super::NotifyDone(context);
}


// #pragma mark - ReaderImplBase


ReaderImplBase::ReaderImplBase(const char* fileType, BErrorOutput* errorOutput)
	:
	fPackageAttributesSection("package attributes"),
	fFileType(fileType),
	fErrorOutput(errorOutput),
	fFile(NULL),
	fOwnsFile(false),
	fRawHeapReader(NULL),
	fHeapReader(NULL),
	fCurrentSection(NULL)
{
}


ReaderImplBase::~ReaderImplBase()
{
	delete fHeapReader;
	if (fRawHeapReader != fHeapReader)
		delete fRawHeapReader;

	if (fOwnsFile)
		delete fFile;
}


uint64
ReaderImplBase::UncompressedHeapSize() const
{
	return fRawHeapReader->UncompressedHeapSize();
}


BAbstractBufferedDataReader*
ReaderImplBase::DetachHeapReader(PackageFileHeapReader*& _rawHeapReader)
{
	BAbstractBufferedDataReader* heapReader = fHeapReader;
	_rawHeapReader = fRawHeapReader;
	fHeapReader = NULL;
	fRawHeapReader = NULL;

	return heapReader;
}


status_t
ReaderImplBase::InitHeapReader(uint32 compression, uint32 chunkSize,
	off_t offset, uint64 compressedSize, uint64 uncompressedSize)
{
	DecompressionAlgorithmOwner* decompressionAlgorithm = NULL;
	BReference<DecompressionAlgorithmOwner> decompressionAlgorithmReference;

	switch (compression) {
		case B_HPKG_COMPRESSION_NONE:
			break;
		case B_HPKG_COMPRESSION_ZLIB:
			decompressionAlgorithm = DecompressionAlgorithmOwner::Create(
				new(std::nothrow) BZlibCompressionAlgorithm,
				new(std::nothrow) BZlibDecompressionParameters);
			decompressionAlgorithmReference.SetTo(decompressionAlgorithm, true);
			if (decompressionAlgorithm == NULL
				|| decompressionAlgorithm->algorithm == NULL
				|| decompressionAlgorithm->parameters == NULL) {
				return B_NO_MEMORY;
			}
			break;
#ifdef ZSTD_ENABLED
		case B_HPKG_COMPRESSION_ZSTD:
			decompressionAlgorithm = DecompressionAlgorithmOwner::Create(
				new(std::nothrow) BZstdCompressionAlgorithm,
				new(std::nothrow) BZstdDecompressionParameters);
			decompressionAlgorithmReference.SetTo(decompressionAlgorithm, true);
			if (decompressionAlgorithm == NULL
				|| decompressionAlgorithm->algorithm == NULL
				|| decompressionAlgorithm->parameters == NULL) {
				return B_NO_MEMORY;
			}
			break;
#endif
		default:
			fErrorOutput->PrintError("Error: Invalid heap compression\n");
			return B_BAD_DATA;
	}

	fRawHeapReader = new(std::nothrow) PackageFileHeapReader(fErrorOutput,
		fFile, offset, compressedSize, uncompressedSize,
		decompressionAlgorithm);
	if (fRawHeapReader == NULL)
		return B_NO_MEMORY;

	status_t error = fRawHeapReader->Init();
	if (error != B_OK)
		return error;

	error = CreateCachedHeapReader(fRawHeapReader, fHeapReader);
	if (error != B_OK) {
		if (error != B_NOT_SUPPORTED)
			return error;

		fHeapReader = fRawHeapReader;
	}

	return B_OK;
}


status_t
ReaderImplBase::CreateCachedHeapReader(PackageFileHeapReader* heapReader,
	BAbstractBufferedDataReader*& _cachedReader)
{
	return B_NOT_SUPPORTED;
}


status_t
ReaderImplBase::InitSection(PackageFileSection& section, uint64 endOffset,
	uint64 length, uint64 maxSaneLength, uint64 stringsLength,
	uint64 stringsCount)
{
	// check length vs. endOffset
	if (length > endOffset) {
		ErrorOutput()->PrintError("Error: %s file %s section size is %"
			B_PRIu64 " bytes. This is greater than the available space\n",
			fFileType, section.name, length);
		return B_BAD_DATA;
	}

	// check sanity length
	if (maxSaneLength > 0 && length > maxSaneLength) {
		ErrorOutput()->PrintError("Error: %s file %s section size is %"
			B_PRIu64 " bytes. This is beyond the reader's sanity limit\n",
			fFileType, section.name, length);
		return B_NOT_SUPPORTED;
	}

	// check strings subsection size/count
	if ((stringsLength <= 1) != (stringsCount == 0) || stringsLength > length) {
		ErrorOutput()->PrintError("Error: strings subsection description of %s "
			"file %s section is invalid (%" B_PRIu64 " strings, length: %"
			B_PRIu64 ", section length: %" B_PRIu64 ")\n",
			fFileType, section.name, stringsCount, stringsLength, length);
		return B_BAD_DATA;
	}

	section.uncompressedLength = length;
	section.offset = endOffset - length;
	section.currentOffset = 0;
	section.stringsLength = stringsLength;
	section.stringsCount = stringsCount;

	return B_OK;
}


status_t
ReaderImplBase::PrepareSection(PackageFileSection& section)
{
	// allocate memory for the section data and read it in
	section.data = new(std::nothrow) uint8[section.uncompressedLength];
	if (section.data == NULL) {
		ErrorOutput()->PrintError("Error: Out of memory!\n");
		return B_NO_MEMORY;
	}

	status_t error = ReadSection(section);
	if (error != B_OK)
		return error;

	// parse the section strings
	section.currentOffset = 0;
	SetCurrentSection(&section);

	error = ParseStrings();
	if (error != B_OK)
		return error;

	return B_OK;
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

	bool sectionHandled;
	status_t error = ParseAttributeTree(context, sectionHandled);
	if (error == B_OK && sectionHandled) {
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
ReaderImplBase::ParseAttributeTree(AttributeHandlerContext* context,
	bool& _sectionHandled)
{
	if (context->hasLowLevelHandler) {
		bool handleSection = false;
		status_t error = context->lowLevelHandler->HandleSectionStart(
			context->section, handleSection);
		if (error != B_OK)
			return error;

		if (!handleSection) {
			_sectionHandled = false;
			return B_OK;
		}
	}

	status_t error = _ParseAttributeTree(context);

	if (context->hasLowLevelHandler) {
		status_t endError = context->lowLevelHandler->HandleSectionEnd(
			context->section);
		if (error == B_OK)
			error = endError;
	}

	_sectionHandled = true;
	return error;
}


status_t
ReaderImplBase::_Init(BPositionIO* file, bool keepFile)
{
	fFile = file;
	fOwnsFile = keepFile;
	return fFile != NULL ? B_OK : B_BAD_VALUE;
}


status_t
ReaderImplBase::_ParseAttributeTree(AttributeHandlerContext* context)
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
			error = handler->NotifyDone(context);
			if (error != B_OK)
				return error;
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
		uint16 type = attribute_tag_type(tag);
		if (type >= B_HPKG_ATTRIBUTE_TYPE_ENUM_COUNT) {
			fErrorOutput->PrintError("Error: Invalid %s section: attribute "
				"type %d not supported!\n", fCurrentSection->name, type);
			return B_BAD_DATA;
		}

		// get the ID
		_id = attribute_tag_id(tag);
		if (_id < B_HPKG_ATTRIBUTE_ID_ENUM_COUNT) {
			if (type != kAttributeTypes[_id]) {
				fErrorOutput->PrintError("Error: Invalid %s section: "
					"unexpected type %d for attribute id %d (expected %d)!\n",
					fCurrentSection->name, type, _id, kAttributeTypes[_id]);
				return B_BAD_DATA;
			}
		} else if (fMinorFormatVersion <= fCurrentMinorFormatVersion) {
			fErrorOutput->PrintError("Error: Invalid %s section: "
				"attribute id %d not supported!\n", fCurrentSection->name, _id);
			return B_BAD_DATA;
		}

		// get the value
		error = ReadAttributeValue(type, attribute_tag_encoding(tag),
			_value);
		if (error != B_OK)
			return error;
	}

	if (_hasChildren != NULL)
		*_hasChildren = attribute_tag_has_children(tag);
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
		fErrorOutput->PrintError(
			"_ReadSectionBuffer(%lu): read beyond %s end\n", size,
			fCurrentSection->name);
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
	status_t error = fFile->ReadAtExactly(offset, buffer, size);
	if (error != B_OK) {
		fErrorOutput->PrintError("_ReadBuffer(%p, %lu) failed to read data: "
			"%s\n", buffer, size, strerror(error));
		return error;
	}

	return B_OK;
}


status_t
ReaderImplBase::ReadSection(const PackageFileSection& section)
{
	return fHeapReader->ReadData(section.offset,
		section.data, section.uncompressedLength);
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
