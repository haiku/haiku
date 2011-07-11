/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/WriterImplBase.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <new>

#include <ByteOrder.h>

#include <AutoDeleter.h>

#include <package/hpkg/HPKGDefsPrivate.h>

#include <package/hpkg/DataReader.h>
#include <package/hpkg/ErrorOutput.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


// #pragma mark - AttributeValue


WriterImplBase::AttributeValue::AttributeValue()
	:
	type(B_HPKG_ATTRIBUTE_TYPE_INVALID),
	encoding(-1)
{
}


WriterImplBase::AttributeValue::~AttributeValue()
{
}


void
WriterImplBase::AttributeValue::SetTo(int8 value)
{
	signedInt = value;
	type = B_HPKG_ATTRIBUTE_TYPE_INT;
}


void
WriterImplBase::AttributeValue::SetTo(uint8 value)
{
	unsignedInt = value;
	type = B_HPKG_ATTRIBUTE_TYPE_UINT;
}


void
WriterImplBase::AttributeValue::SetTo(int16 value)
{
	signedInt = value;
	type = B_HPKG_ATTRIBUTE_TYPE_INT;
}


void
WriterImplBase::AttributeValue::SetTo(uint16 value)
{
	unsignedInt = value;
	type = B_HPKG_ATTRIBUTE_TYPE_UINT;
}


void
WriterImplBase::AttributeValue::SetTo(int32 value)
{
	signedInt = value;
	type = B_HPKG_ATTRIBUTE_TYPE_INT;
}


void
WriterImplBase::AttributeValue::SetTo(uint32 value)
{
	unsignedInt = value;
	type = B_HPKG_ATTRIBUTE_TYPE_UINT;
}


void
WriterImplBase::AttributeValue::SetTo(int64 value)
{
	signedInt = value;
	type = B_HPKG_ATTRIBUTE_TYPE_INT;
}


void
WriterImplBase::AttributeValue::SetTo(uint64 value)
{
	unsignedInt = value;
	type = B_HPKG_ATTRIBUTE_TYPE_UINT;
}


void
WriterImplBase::AttributeValue::SetTo(CachedString* value)
{
	string = value;
	type = B_HPKG_ATTRIBUTE_TYPE_STRING;
}


void
WriterImplBase::AttributeValue::SetToData(uint64 size, uint64 offset)
{
	data.size = size;
	data.offset = offset;
	type = B_HPKG_ATTRIBUTE_TYPE_RAW;
	encoding = B_HPKG_ATTRIBUTE_ENCODING_RAW_HEAP;
}


void
WriterImplBase::AttributeValue::SetToData(uint64 size, const void* rawData)
{
	data.size = size;
	if (size > 0)
		memcpy(data.raw, rawData, size);
	type = B_HPKG_ATTRIBUTE_TYPE_RAW;
	encoding = B_HPKG_ATTRIBUTE_ENCODING_RAW_INLINE;
}


uint8
WriterImplBase::AttributeValue::ApplicableEncoding() const
{
	switch (type) {
		case B_HPKG_ATTRIBUTE_TYPE_INT:
			return _ApplicableIntEncoding(signedInt >= 0
				? (uint64)signedInt << 1
				: (uint64)(-(signedInt + 1) << 1));
		case B_HPKG_ATTRIBUTE_TYPE_UINT:
			return _ApplicableIntEncoding(unsignedInt);
		case B_HPKG_ATTRIBUTE_TYPE_STRING:
			return string->index >= 0
				? B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE
				: B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE;
		case B_HPKG_ATTRIBUTE_TYPE_RAW:
			return encoding;
		default:
			return 0;
	}
}


/*static*/ uint8
WriterImplBase::AttributeValue::_ApplicableIntEncoding(uint64 value)
{
	if (value <= 0xff)
		return B_HPKG_ATTRIBUTE_ENCODING_INT_8_BIT;
	if (value <= 0xffff)
		return B_HPKG_ATTRIBUTE_ENCODING_INT_16_BIT;
	if (value <= 0xffffffff)
		return B_HPKG_ATTRIBUTE_ENCODING_INT_32_BIT;

	return B_HPKG_ATTRIBUTE_ENCODING_INT_64_BIT;
}


// #pragma mark - AbstractDataWriter


WriterImplBase::AbstractDataWriter::AbstractDataWriter()
	:
	fBytesWritten(0)
{
}


WriterImplBase::AbstractDataWriter::~AbstractDataWriter()
{
}


// #pragma mark - FDDataWriter


WriterImplBase::FDDataWriter::FDDataWriter(int fd, off_t offset,
	BErrorOutput* errorOutput)
	:
	fFD(fd),
	fOffset(offset),
	fErrorOutput(errorOutput)
{
}


status_t
WriterImplBase::FDDataWriter::WriteDataNoThrow(const void* buffer, size_t size)
{
	ssize_t bytesWritten = pwrite(fFD, buffer, size, fOffset);
	if (bytesWritten < 0) {
		fErrorOutput->PrintError(
			"WriteDataNoThrow(%p, %lu) failed to write data: %s\n", buffer,
			size, strerror(errno));
		return errno;
	}
	if ((size_t)bytesWritten != size) {
		fErrorOutput->PrintError(
			"WriteDataNoThrow(%p, %lu) failed to write all data\n", buffer,
			size);
		return B_ERROR;
	}

	fOffset += size;
	fBytesWritten += size;
	return B_OK;
}


// #pragma mark - ZlibDataWriter


WriterImplBase::ZlibDataWriter::ZlibDataWriter(AbstractDataWriter* dataWriter)
	:
	fDataWriter(dataWriter),
	fCompressor(this)
{
}


void
WriterImplBase::ZlibDataWriter::Init()
{
	status_t error = fCompressor.Init();
	if (error != B_OK)
		throw status_t(error);
}


void
WriterImplBase::ZlibDataWriter::Finish()
{
	status_t error = fCompressor.Finish();
	if (error != B_OK)
		throw status_t(error);
}


status_t
WriterImplBase::ZlibDataWriter::WriteDataNoThrow(const void* buffer,
	size_t size)
{
	status_t error = fCompressor.CompressNext(buffer, size);
	if (error == B_OK)
		fBytesWritten += size;
	return error;
}


status_t
WriterImplBase::ZlibDataWriter::WriteData(const void* buffer, size_t size)
{
	return fDataWriter->WriteDataNoThrow(buffer, size);
}


// #pragma mark - PackageAttribute

WriterImplBase::PackageAttribute::PackageAttribute(BHPKGAttributeID id_,
	uint8 type_, uint8 encoding_)
	:
	id(id_)
{
	type = type_;
	encoding = encoding_;
}


WriterImplBase::PackageAttribute::~PackageAttribute()
{
	_DeleteChildren();
}


void
WriterImplBase::PackageAttribute::AddChild(PackageAttribute* child)
{
	children.Add(child);
}


void
WriterImplBase::PackageAttribute::_DeleteChildren()
{
	while (PackageAttribute* child = children.RemoveHead())
		delete child;
}


// #pragma mark - WriterImplBase


WriterImplBase::WriterImplBase(BErrorOutput* errorOutput)
	:
	fErrorOutput(errorOutput),
	fFileName(NULL),
	fFlags(0),
	fFD(-1),
	fFinished(false),
	fDataWriter(NULL)
{
}


WriterImplBase::~WriterImplBase()
{
	if (fFD >= 0)
		close(fFD);

	if (!fFinished && fFileName != NULL
		&& (fFlags & B_HPKG_WRITER_UPDATE_PACKAGE) == 0) {
		unlink(fFileName);
	}
}


status_t
WriterImplBase::Init(const char* fileName, const char* type, uint32 flags)
{
	if (fPackageStringCache.Init() != B_OK)
		throw std::bad_alloc();

	// open file (don't truncate in update mode)
	int openMode = O_RDWR;
	if ((flags & B_HPKG_WRITER_UPDATE_PACKAGE) == 0)
		openMode |= O_CREAT | O_TRUNC;

	fFD = open(fileName, openMode, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fFD < 0) {
		fErrorOutput->PrintError("Failed to open %s file \"%s\": %s\n", type,
			fileName, strerror(errno));
		return errno;
	}

	fFileName = fileName;
	fFlags = flags;

	return B_OK;
}


void
WriterImplBase::RegisterPackageInfo(PackageAttributeList& attributeList,
	const BPackageInfo& packageInfo)
{
	// name
	PackageAttribute* name = new PackageAttribute(
		B_HPKG_ATTRIBUTE_ID_PACKAGE_NAME, B_HPKG_ATTRIBUTE_TYPE_STRING,
		B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
	name->string = fPackageStringCache.Get(packageInfo.Name().String());
	attributeList.Add(name);

	// summary
	PackageAttribute* summary = new PackageAttribute(
		B_HPKG_ATTRIBUTE_ID_PACKAGE_SUMMARY, B_HPKG_ATTRIBUTE_TYPE_STRING,
		B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
	summary->string = fPackageStringCache.Get(packageInfo.Summary().String());
	attributeList.Add(summary);

	// description
	PackageAttribute* description = new PackageAttribute(
		B_HPKG_ATTRIBUTE_ID_PACKAGE_DESCRIPTION, B_HPKG_ATTRIBUTE_TYPE_STRING,
		B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
	description->string
		= fPackageStringCache.Get(packageInfo.Description().String());
	attributeList.Add(description);

	// vendor
	PackageAttribute* vendor = new PackageAttribute(
		B_HPKG_ATTRIBUTE_ID_PACKAGE_VENDOR, B_HPKG_ATTRIBUTE_TYPE_STRING,
		B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
	vendor->string = fPackageStringCache.Get(packageInfo.Vendor().String());
	attributeList.Add(vendor);

	// packager
	PackageAttribute* packager = new PackageAttribute(
		B_HPKG_ATTRIBUTE_ID_PACKAGE_PACKAGER, B_HPKG_ATTRIBUTE_TYPE_STRING,
		B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
	packager->string = fPackageStringCache.Get(packageInfo.Packager().String());
	attributeList.Add(packager);

	// flags
	PackageAttribute* flags = new PackageAttribute(
		B_HPKG_ATTRIBUTE_ID_PACKAGE_FLAGS, B_HPKG_ATTRIBUTE_TYPE_UINT,
		B_HPKG_ATTRIBUTE_ENCODING_INT_32_BIT);
	flags->unsignedInt = packageInfo.Flags();
	attributeList.Add(flags);

	// architecture
	PackageAttribute* architecture = new PackageAttribute(
		B_HPKG_ATTRIBUTE_ID_PACKAGE_ARCHITECTURE, B_HPKG_ATTRIBUTE_TYPE_UINT,
		B_HPKG_ATTRIBUTE_ENCODING_INT_8_BIT);
	architecture->unsignedInt = packageInfo.Architecture();
	attributeList.Add(architecture);

	// version
	RegisterPackageVersion(attributeList, packageInfo.Version());

	// copyright list
	const BObjectList<BString>& copyrightList = packageInfo.CopyrightList();
	for (int i = 0; i < copyrightList.CountItems(); ++i) {
		PackageAttribute* copyright = new PackageAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_COPYRIGHT, B_HPKG_ATTRIBUTE_TYPE_STRING,
			B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
		copyright->string
			= fPackageStringCache.Get(copyrightList.ItemAt(i)->String());
		attributeList.Add(copyright);
	}

	// license list
	const BObjectList<BString>& licenseList = packageInfo.LicenseList();
	for (int i = 0; i < licenseList.CountItems(); ++i) {
		PackageAttribute* license = new PackageAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_LICENSE, B_HPKG_ATTRIBUTE_TYPE_STRING,
			B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
		license->string
			= fPackageStringCache.Get(licenseList.ItemAt(i)->String());
		attributeList.Add(license);
	}

	// URL list
	const BObjectList<BString>& urlList = packageInfo.URLList();
	for (int i = 0; i < urlList.CountItems(); ++i) {
		PackageAttribute* url = new PackageAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_URL, B_HPKG_ATTRIBUTE_TYPE_STRING,
			B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
		url->string = fPackageStringCache.Get(urlList.ItemAt(i)->String());
		attributeList.Add(url);
	}

	// source URL list
	const BObjectList<BString>& sourceURLList = packageInfo.SourceURLList();
	for (int i = 0; i < sourceURLList.CountItems(); ++i) {
		PackageAttribute* url = new PackageAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_SOURCE_URL,
			B_HPKG_ATTRIBUTE_TYPE_STRING,
			B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
		url->string = fPackageStringCache.Get(
			sourceURLList.ItemAt(i)->String());
		attributeList.Add(url);
	}

	// provides list
	const BObjectList<BPackageResolvable>& providesList
		= packageInfo.ProvidesList();
	for (int i = 0; i < providesList.CountItems(); ++i) {
		BPackageResolvable* resolvable = providesList.ItemAt(i);
		bool hasVersion = resolvable->Version().InitCheck() == B_OK;
		bool hasCompatibleVersion
			= resolvable->CompatibleVersion().InitCheck() == B_OK;

		PackageAttribute* provides = new PackageAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_PROVIDES, B_HPKG_ATTRIBUTE_TYPE_STRING,
			B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
		provides->string = fPackageStringCache.Get(resolvable->Name().String());
		attributeList.Add(provides);

		PackageAttribute* providesType = new PackageAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_PROVIDES_TYPE,
			B_HPKG_ATTRIBUTE_TYPE_UINT, B_HPKG_ATTRIBUTE_ENCODING_INT_8_BIT);
		providesType->unsignedInt = resolvable->Type();
		provides->children.Add(providesType);

		if (hasVersion)
			RegisterPackageVersion(provides->children, resolvable->Version());

		if (hasCompatibleVersion) {
			RegisterPackageVersion(provides->children,
				resolvable->CompatibleVersion(),
				B_HPKG_ATTRIBUTE_ID_PACKAGE_PROVIDES_COMPATIBLE);
		}
	}

	// requires list
	RegisterPackageResolvableExpressionList(attributeList,
		packageInfo.RequiresList(), B_HPKG_ATTRIBUTE_ID_PACKAGE_REQUIRES);

	// supplements list
	RegisterPackageResolvableExpressionList(attributeList,
		packageInfo.SupplementsList(), B_HPKG_ATTRIBUTE_ID_PACKAGE_SUPPLEMENTS);

	// conflicts list
	RegisterPackageResolvableExpressionList(attributeList,
		packageInfo.ConflictsList(), B_HPKG_ATTRIBUTE_ID_PACKAGE_CONFLICTS);

	// freshens list
	RegisterPackageResolvableExpressionList(attributeList,
		packageInfo.FreshensList(), B_HPKG_ATTRIBUTE_ID_PACKAGE_FRESHENS);

	// replaces list
	const BObjectList<BString>& replacesList = packageInfo.ReplacesList();
	for (int i = 0; i < replacesList.CountItems(); ++i) {
		PackageAttribute* replaces = new PackageAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_REPLACES, B_HPKG_ATTRIBUTE_TYPE_STRING,
			B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
		replaces->string
			= fPackageStringCache.Get(replacesList.ItemAt(i)->String());
		attributeList.Add(replaces);
	}

	// checksum (optional, only exists in repositories)
	if (packageInfo.Checksum().Length() > 0) {
		PackageAttribute* checksum = new PackageAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_CHECKSUM, B_HPKG_ATTRIBUTE_TYPE_STRING,
			B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
		checksum->string
			= fPackageStringCache.Get(packageInfo.Checksum().String());
		attributeList.Add(checksum);
	}

	// install path (optional)
	if (!packageInfo.InstallPath().IsEmpty()) {
		PackageAttribute* installPath = new PackageAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_INSTALL_PATH,
			B_HPKG_ATTRIBUTE_TYPE_STRING,
			B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
		installPath->string = fPackageStringCache.Get(
			packageInfo.InstallPath().String());
		attributeList.Add(installPath);
	}
}


void
WriterImplBase::RegisterPackageVersion(PackageAttributeList& attributeList,
	const BPackageVersion& version, BHPKGAttributeID attributeID)
{
	PackageAttribute* versionMajor = new PackageAttribute(
		attributeID, B_HPKG_ATTRIBUTE_TYPE_STRING,
		B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
	versionMajor->string = fPackageStringCache.Get(version.Major().String());
	attributeList.Add(versionMajor);

	if (version.Minor().Length() > 0) {
		PackageAttribute* versionMinor = new PackageAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MINOR,
			B_HPKG_ATTRIBUTE_TYPE_STRING,
			B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
		versionMinor->string
			= fPackageStringCache.Get(version.Minor().String());
		versionMajor->children.Add(versionMinor);

		if (version.Micro().Length() > 0) {
			PackageAttribute* versionMicro = new PackageAttribute(
				B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MICRO,
				B_HPKG_ATTRIBUTE_TYPE_STRING,
				B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
			versionMicro->string
				= fPackageStringCache.Get(version.Micro().String());
			versionMajor->children.Add(versionMicro);
		}
	}

	if (!version.PreRelease().IsEmpty()) {
		PackageAttribute* preRelease = new PackageAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_PRE_RELEASE,
			B_HPKG_ATTRIBUTE_TYPE_STRING,
			B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
		preRelease->string
			= fPackageStringCache.Get(version.PreRelease().String());
		versionMajor->children.Add(preRelease);
	}

	if (version.Release() != 0) {
		PackageAttribute* versionRelease = new PackageAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_RELEASE,
			B_HPKG_ATTRIBUTE_TYPE_UINT, B_HPKG_ATTRIBUTE_ENCODING_INT_8_BIT);
		versionRelease->unsignedInt = version.Release();
		versionMajor->children.Add(versionRelease);
	}
}


void
WriterImplBase::RegisterPackageResolvableExpressionList(
	PackageAttributeList& attributeList,
	const BObjectList<BPackageResolvableExpression>& expressionList, uint8 id)
{
	for (int i = 0; i < expressionList.CountItems(); ++i) {
		BPackageResolvableExpression* resolvableExpr = expressionList.ItemAt(i);
		bool hasVersion = resolvableExpr->Version().InitCheck() == B_OK;

		PackageAttribute* name = new PackageAttribute((BHPKGAttributeID)id,
			B_HPKG_ATTRIBUTE_TYPE_STRING,
			B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
		name->string = fPackageStringCache.Get(resolvableExpr->Name().String());
		attributeList.Add(name);

		if (hasVersion) {
			PackageAttribute* op = new PackageAttribute(
				B_HPKG_ATTRIBUTE_ID_PACKAGE_RESOLVABLE_OPERATOR,
				B_HPKG_ATTRIBUTE_TYPE_UINT,
				B_HPKG_ATTRIBUTE_ENCODING_INT_8_BIT);
			op->unsignedInt = resolvableExpr->Operator();
			name->children.Add(op);
			RegisterPackageVersion(name->children, resolvableExpr->Version());
		}
	}
}


int32
WriterImplBase::WriteCachedStrings(const StringCache& cache,
	uint32 minUsageCount)
{
	// create an array of the cached strings
	int32 count = cache.CountElements();
	CachedString** cachedStrings = new CachedString*[count];
	ArrayDeleter<CachedString*> cachedStringsDeleter(cachedStrings);

	int32 index = 0;
	for (CachedStringTable::Iterator it = cache.GetIterator();
			CachedString* string = it.Next();) {
		cachedStrings[index++] = string;
	}

	// sort it by descending usage count
	std::sort(cachedStrings, cachedStrings + count, CachedStringUsageGreater());

	// assign the indices and write entries to disk
	int32 stringsWritten = 0;
	for (int32 i = 0; i < count; i++) {
		CachedString* cachedString = cachedStrings[i];

		// empty strings must be stored inline, as they can't be distinguished
		// from the end-marker!
		if (strlen(cachedString->string) == 0)
			continue;

		// strings that are used only once are better stored inline
		if (cachedString->usageCount < minUsageCount)
			break;

		WriteString(cachedString->string);

		cachedString->index = stringsWritten++;
	}

	// write a terminating 0 byte
	Write<uint8>(0);

	return stringsWritten;
}


int32
WriterImplBase::WritePackageAttributes(
	const PackageAttributeList& packageAttributes,
	uint32& _stringsLengthUncompressed)
{
	// write the cached strings
	uint32 stringsCount = WriteCachedStrings(fPackageStringCache, 2);
	_stringsLengthUncompressed = DataWriter()->BytesWritten();

	_WritePackageAttributes(packageAttributes);

	return stringsCount;
}


void
WriterImplBase::WriteAttributeValue(const AttributeValue& value, uint8 encoding)
{
	switch (value.type) {
		case B_HPKG_ATTRIBUTE_TYPE_INT:
		case B_HPKG_ATTRIBUTE_TYPE_UINT:
		{
			uint64 intValue = value.type == B_HPKG_ATTRIBUTE_TYPE_INT
				? (uint64)value.signedInt : value.unsignedInt;

			switch (encoding) {
				case B_HPKG_ATTRIBUTE_ENCODING_INT_8_BIT:
					Write<uint8>((uint8)intValue);
					break;
				case B_HPKG_ATTRIBUTE_ENCODING_INT_16_BIT:
					Write<uint16>(
						B_HOST_TO_BENDIAN_INT16((uint16)intValue));
					break;
				case B_HPKG_ATTRIBUTE_ENCODING_INT_32_BIT:
					Write<uint32>(
						B_HOST_TO_BENDIAN_INT32((uint32)intValue));
					break;
				case B_HPKG_ATTRIBUTE_ENCODING_INT_64_BIT:
					Write<uint64>(
						B_HOST_TO_BENDIAN_INT64((uint64)intValue));
					break;
				default:
				{
					fErrorOutput->PrintError("WriteAttributeValue(): invalid "
						"encoding %d for int value type %d\n", encoding,
						value.type);
					throw status_t(B_BAD_VALUE);
				}
			}

			break;
		}

		case B_HPKG_ATTRIBUTE_TYPE_STRING:
		{
			if (encoding == B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE)
				WriteUnsignedLEB128(value.string->index);
			else
				WriteString(value.string->string);
			break;
		}

		case B_HPKG_ATTRIBUTE_TYPE_RAW:
		{
			WriteUnsignedLEB128(value.data.size);
			if (encoding == B_HPKG_ATTRIBUTE_ENCODING_RAW_HEAP)
				WriteUnsignedLEB128(value.data.offset);
			else
				fDataWriter->WriteDataThrows(value.data.raw, value.data.size);
			break;
		}

		default:
			fErrorOutput->PrintError(
				"WriteAttributeValue(): invalid value type: %d\n", value.type);
			throw status_t(B_BAD_VALUE);
	}
}


void
WriterImplBase::WriteUnsignedLEB128(uint64 value)
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
WriterImplBase::WriteBuffer(const void* buffer, size_t size, off_t offset)
{
	ssize_t bytesWritten = pwrite(fFD, buffer, size, offset);
	if (bytesWritten < 0) {
		fErrorOutput->PrintError(
			"WriteBuffer(%p, %lu) failed to write data: %s\n", buffer, size,
			strerror(errno));
		throw status_t(errno);
	}
	if ((size_t)bytesWritten != size) {
		fErrorOutput->PrintError(
			"WriteBuffer(%p, %lu) failed to write all data\n", buffer, size);
		throw status_t(B_ERROR);
	}
}


void
WriterImplBase::_WritePackageAttributes(
	const PackageAttributeList& packageAttributes)
{
	DoublyLinkedList<PackageAttribute>::ConstIterator it
		= packageAttributes.GetIterator();
	while (PackageAttribute* attribute = it.Next()) {
		uint8 encoding = attribute->ApplicableEncoding();

		// write tag
		WriteUnsignedLEB128(HPKG_ATTRIBUTE_TAG_COMPOSE(
			attribute->id, attribute->type, encoding,
			!attribute->children.IsEmpty()));

		// write value
		WriteAttributeValue(*attribute, encoding);

		if (!attribute->children.IsEmpty())
			_WritePackageAttributes(attribute->children);
	}

	WriteUnsignedLEB128(0);
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
