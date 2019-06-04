/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/WriterImplBase.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <new>

#include <ByteOrder.h>
#include <File.h>

#include <AutoDeleter.h>
#include <ZlibCompressionAlgorithm.h>
#ifdef ZSTD_ENABLED
#include <ZstdCompressionAlgorithm.h>
#endif

#include <package/hpkg/DataReader.h>
#include <package/hpkg/ErrorOutput.h>

#include <package/hpkg/HPKGDefsPrivate.h>


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


WriterImplBase::WriterImplBase(const char* fileType, BErrorOutput* errorOutput)
	:
	fHeapWriter(NULL),
	fCompressionAlgorithm(NULL),
	fCompressionParameters(NULL),
	fDecompressionAlgorithm(NULL),
	fDecompressionParameters(NULL),
	fFileType(fileType),
	fErrorOutput(errorOutput),
	fFileName(NULL),
	fParameters(),
	fFile(NULL),
	fOwnsFile(false),
	fFinished(false)
{
}


WriterImplBase::~WriterImplBase()
{
	delete fHeapWriter;
	delete fCompressionAlgorithm;
	delete fCompressionParameters;
	delete fDecompressionAlgorithm;
	delete fDecompressionParameters;

	if (fOwnsFile)
		delete fFile;

	if (!fFinished && fFileName != NULL
		&& (Flags() & B_HPKG_WRITER_UPDATE_PACKAGE) == 0) {
		unlink(fFileName);
	}
}


status_t
WriterImplBase::Init(BPositionIO* file, bool keepFile, const char* fileName,
	const BPackageWriterParameters& parameters)
{
	fParameters = parameters;

	if (fPackageStringCache.Init() != B_OK)
		throw std::bad_alloc();

	if (file == NULL) {
		if (fileName == NULL)
			return B_BAD_VALUE;

		// open file (don't truncate in update mode)
		int openMode = O_RDWR;
		if ((parameters.Flags() & B_HPKG_WRITER_UPDATE_PACKAGE) == 0)
			openMode |= O_CREAT | O_TRUNC;

		BFile* newFile = new BFile;
		status_t error = newFile->SetTo(fileName, openMode);
		if (error != B_OK) {
			fErrorOutput->PrintError("Failed to open %s file \"%s\": %s\n",
				fFileType, fileName, strerror(errno));
			delete newFile;
			return error;
		}

		fFile = newFile;
		fOwnsFile = true;
	} else {
		fFile = file;
		fOwnsFile = keepFile;
	}

	fFileName = fileName;

	return B_OK;
}


status_t
WriterImplBase::InitHeapReader(size_t headerSize)
{
	// allocate the compression/decompression algorithm
	CompressionAlgorithmOwner* compressionAlgorithm = NULL;
	BReference<CompressionAlgorithmOwner> compressionAlgorithmReference;

	DecompressionAlgorithmOwner* decompressionAlgorithm = NULL;
	BReference<DecompressionAlgorithmOwner> decompressionAlgorithmReference;

	switch (fParameters.Compression()) {
		case B_HPKG_COMPRESSION_NONE:
			break;
		case B_HPKG_COMPRESSION_ZLIB:
			compressionAlgorithm = CompressionAlgorithmOwner::Create(
				new(std::nothrow) BZlibCompressionAlgorithm,
				new(std::nothrow) BZlibCompressionParameters(
					fParameters.CompressionLevel()));
			compressionAlgorithmReference.SetTo(compressionAlgorithm, true);

			decompressionAlgorithm = DecompressionAlgorithmOwner::Create(
				new(std::nothrow) BZlibCompressionAlgorithm,
				new(std::nothrow) BZlibDecompressionParameters);
			decompressionAlgorithmReference.SetTo(decompressionAlgorithm, true);

			if (compressionAlgorithm == NULL
				|| compressionAlgorithm->algorithm == NULL
				|| compressionAlgorithm->parameters == NULL
				|| decompressionAlgorithm == NULL
				|| decompressionAlgorithm->algorithm == NULL
				|| decompressionAlgorithm->parameters == NULL) {
				throw std::bad_alloc();
			}
			break;
#ifdef ZSTD_ENABLED
		case B_HPKG_COMPRESSION_ZSTD:
			compressionAlgorithm = CompressionAlgorithmOwner::Create(
				new(std::nothrow) BZstdCompressionAlgorithm,
				new(std::nothrow) BZstdCompressionParameters(
					fParameters.CompressionLevel()));
			compressionAlgorithmReference.SetTo(compressionAlgorithm, true);

			decompressionAlgorithm = DecompressionAlgorithmOwner::Create(
				new(std::nothrow) BZstdCompressionAlgorithm,
				new(std::nothrow) BZstdDecompressionParameters);
			decompressionAlgorithmReference.SetTo(decompressionAlgorithm, true);

			if (compressionAlgorithm == NULL
				|| compressionAlgorithm->algorithm == NULL
				|| compressionAlgorithm->parameters == NULL
				|| decompressionAlgorithm == NULL
				|| decompressionAlgorithm->algorithm == NULL
				|| decompressionAlgorithm->parameters == NULL) {
				throw std::bad_alloc();
			}
			break;
#endif
		default:
			fErrorOutput->PrintError("Error: Invalid heap compression\n");
			return B_BAD_VALUE;
	}

	// create heap writer
	fHeapWriter = new PackageFileHeapWriter(fErrorOutput, fFile, headerSize,
		compressionAlgorithm, decompressionAlgorithm);
	fHeapWriter->Init();

	return B_OK;
}


void
WriterImplBase::SetCompression(uint32 compression)
{
	fParameters.SetCompression(compression);
}


void
WriterImplBase::RegisterPackageInfo(PackageAttributeList& attributeList,
	const BPackageInfo& packageInfo)
{
	// name
	AddStringAttribute(B_HPKG_ATTRIBUTE_ID_PACKAGE_NAME, packageInfo.Name(),
		attributeList);

	// summary
	AddStringAttribute(B_HPKG_ATTRIBUTE_ID_PACKAGE_SUMMARY,
		packageInfo.Summary(), attributeList);

	// description
	AddStringAttribute(B_HPKG_ATTRIBUTE_ID_PACKAGE_DESCRIPTION,
		packageInfo.Description(), attributeList);

	// vendor
	AddStringAttribute(B_HPKG_ATTRIBUTE_ID_PACKAGE_VENDOR,
		packageInfo.Vendor(), attributeList);

	// packager
	AddStringAttribute(B_HPKG_ATTRIBUTE_ID_PACKAGE_PACKAGER,
		packageInfo.Packager(), attributeList);

	// base package (optional)
	_AddStringAttributeIfNotEmpty(B_HPKG_ATTRIBUTE_ID_PACKAGE_BASE_PACKAGE,
		packageInfo.BasePackage(), attributeList);

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
	_AddStringAttributeList(B_HPKG_ATTRIBUTE_ID_PACKAGE_COPYRIGHT,
			packageInfo.CopyrightList(), attributeList);

	// license list
	_AddStringAttributeList(B_HPKG_ATTRIBUTE_ID_PACKAGE_LICENSE,
		packageInfo.LicenseList(), attributeList);

	// URL list
	_AddStringAttributeList(B_HPKG_ATTRIBUTE_ID_PACKAGE_URL,
		packageInfo.URLList(), attributeList);

	// source URL list
	_AddStringAttributeList(B_HPKG_ATTRIBUTE_ID_PACKAGE_SOURCE_URL,
		packageInfo.SourceURLList(), attributeList);

	// provides list
	const BObjectList<BPackageResolvable>& providesList
		= packageInfo.ProvidesList();
	for (int i = 0; i < providesList.CountItems(); ++i) {
		BPackageResolvable* resolvable = providesList.ItemAt(i);
		bool hasVersion = resolvable->Version().InitCheck() == B_OK;
		bool hasCompatibleVersion
			= resolvable->CompatibleVersion().InitCheck() == B_OK;

		PackageAttribute* provides = AddStringAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_PROVIDES, resolvable->Name(),
			attributeList);

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
	_AddStringAttributeList(B_HPKG_ATTRIBUTE_ID_PACKAGE_REPLACES,
		packageInfo.ReplacesList(), attributeList);

	// global writable file info list
	const BObjectList<BGlobalWritableFileInfo>& globalWritableFileInfos
		= packageInfo.GlobalWritableFileInfos();
	for (int32 i = 0; i < globalWritableFileInfos.CountItems(); ++i) {
		BGlobalWritableFileInfo* info = globalWritableFileInfos.ItemAt(i);
		PackageAttribute* attribute = AddStringAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_GLOBAL_WRITABLE_FILE, info->Path(),
			attributeList);

		if (info->IsDirectory()) {
			PackageAttribute* isDirectoryAttribute = new PackageAttribute(
				B_HPKG_ATTRIBUTE_ID_PACKAGE_IS_WRITABLE_DIRECTORY,
				B_HPKG_ATTRIBUTE_TYPE_UINT,
				B_HPKG_ATTRIBUTE_ENCODING_INT_8_BIT);
			isDirectoryAttribute->unsignedInt = 1;
			attribute->children.Add(isDirectoryAttribute);
		}

		if (info->IsIncluded()) {
			PackageAttribute* updateTypeAttribute = new PackageAttribute(
				B_HPKG_ATTRIBUTE_ID_PACKAGE_WRITABLE_FILE_UPDATE_TYPE,
				B_HPKG_ATTRIBUTE_TYPE_UINT,
				B_HPKG_ATTRIBUTE_ENCODING_INT_8_BIT);
			updateTypeAttribute->unsignedInt = info->UpdateType();
			attribute->children.Add(updateTypeAttribute);
		}
	}

	// user settings file info list
	const BObjectList<BUserSettingsFileInfo>& userSettingsFileInfos
		= packageInfo.UserSettingsFileInfos();
	for (int32 i = 0; i < userSettingsFileInfos.CountItems(); ++i) {
		BUserSettingsFileInfo* info = userSettingsFileInfos.ItemAt(i);
		PackageAttribute* attribute = AddStringAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_SETTINGS_FILE, info->Path(),
			attributeList);

		if (info->IsDirectory()) {
			PackageAttribute* isDirectoryAttribute = new PackageAttribute(
				B_HPKG_ATTRIBUTE_ID_PACKAGE_IS_WRITABLE_DIRECTORY,
				B_HPKG_ATTRIBUTE_TYPE_UINT,
				B_HPKG_ATTRIBUTE_ENCODING_INT_8_BIT);
			isDirectoryAttribute->unsignedInt = 1;
			attribute->children.Add(isDirectoryAttribute);
		} else {
			_AddStringAttributeIfNotEmpty(
				B_HPKG_ATTRIBUTE_ID_PACKAGE_SETTINGS_FILE_TEMPLATE,
				info->TemplatePath(), attribute->children);
		}
	}

	// user list
	const BObjectList<BUser>& users = packageInfo.Users();
	for (int32 i = 0; i < users.CountItems(); ++i) {
		const BUser* user = users.ItemAt(i);
		PackageAttribute* attribute = AddStringAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_USER, user->Name(), attributeList);

		_AddStringAttributeIfNotEmpty(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_REAL_NAME, user->RealName(),
			attribute->children);
		_AddStringAttributeIfNotEmpty(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_HOME, user->Home(),
			attribute->children);
		_AddStringAttributeIfNotEmpty(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_SHELL, user->Shell(),
			attribute->children);

		for (int32 k = 0; k < user->Groups().CountStrings(); k++) {
			AddStringAttribute(B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_GROUP,
				user->Groups().StringAt(k), attribute->children);
		}
	}

	// group list
	_AddStringAttributeList(B_HPKG_ATTRIBUTE_ID_PACKAGE_GROUP,
		packageInfo.Groups(), attributeList);

	// post install script list
	_AddStringAttributeList(B_HPKG_ATTRIBUTE_ID_PACKAGE_POST_INSTALL_SCRIPT,
		packageInfo.PostInstallScripts(), attributeList);

	// pre uninstall script list
	_AddStringAttributeList(B_HPKG_ATTRIBUTE_ID_PACKAGE_PRE_UNINSTALL_SCRIPT,
		packageInfo.PreUninstallScripts(), attributeList);

	// checksum (optional, only exists in repositories)
	_AddStringAttributeIfNotEmpty(B_HPKG_ATTRIBUTE_ID_PACKAGE_CHECKSUM,
		packageInfo.Checksum(), attributeList);

	// install path (optional)
	_AddStringAttributeIfNotEmpty(B_HPKG_ATTRIBUTE_ID_PACKAGE_INSTALL_PATH,
		packageInfo.InstallPath(), attributeList);
}


void
WriterImplBase::RegisterPackageVersion(PackageAttributeList& attributeList,
	const BPackageVersion& version, BHPKGAttributeID attributeID)
{
	PackageAttribute* versionMajor = AddStringAttribute(attributeID,
		version.Major(), attributeList);

	if (!version.Minor().IsEmpty()) {
		AddStringAttribute(B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MINOR,
			version.Minor(), versionMajor->children);
		_AddStringAttributeIfNotEmpty(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MICRO, version.Micro(),
			versionMajor->children);
	}

	_AddStringAttributeIfNotEmpty(
		B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_PRE_RELEASE,
		version.PreRelease(), versionMajor->children);

	if (version.Revision() != 0) {
		PackageAttribute* versionRevision = new PackageAttribute(
			B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_REVISION,
			B_HPKG_ATTRIBUTE_TYPE_UINT, B_HPKG_ATTRIBUTE_ENCODING_INT_32_BIT);
		versionRevision->unsignedInt = version.Revision();
		versionMajor->children.Add(versionRevision);
	}
}


void
WriterImplBase::RegisterPackageResolvableExpressionList(
	PackageAttributeList& attributeList,
	const BObjectList<BPackageResolvableExpression>& expressionList, uint8 id)
{
	for (int i = 0; i < expressionList.CountItems(); ++i) {
		BPackageResolvableExpression* resolvableExpr = expressionList.ItemAt(i);
		PackageAttribute* name = AddStringAttribute((BHPKGAttributeID)id,
			resolvableExpr->Name(), attributeList);

		if (resolvableExpr->Version().InitCheck() == B_OK) {
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


WriterImplBase::PackageAttribute*
WriterImplBase::AddStringAttribute(BHPKGAttributeID id, const BString& value,
	DoublyLinkedList<PackageAttribute>& list)
{
	PackageAttribute* attribute = new PackageAttribute(id,
		B_HPKG_ATTRIBUTE_TYPE_STRING, B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE);
	attribute->string = fPackageStringCache.Get(value);
	list.Add(attribute);
	return attribute;
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
	uint64 startOffset = fHeapWriter->UncompressedHeapSize();
	uint32 stringsCount = WriteCachedStrings(fPackageStringCache, 2);
	_stringsLengthUncompressed
		= fHeapWriter->UncompressedHeapSize() - startOffset;

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
				fHeapWriter->AddDataThrows(value.data.raw, value.data.size);
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

	fHeapWriter->AddDataThrows(bytes, count);
}


void
WriterImplBase::RawWriteBuffer(const void* buffer, size_t size, off_t offset)
{
	status_t error = fFile->WriteAtExactly(offset, buffer, size);
	if (error != B_OK) {
		fErrorOutput->PrintError(
			"RawWriteBuffer(%p, %lu) failed to write data: %s\n", buffer, size,
			strerror(error));
		throw error;
	}
}


void
WriterImplBase::_AddStringAttributeList(BHPKGAttributeID id,
	const BStringList& value, DoublyLinkedList<PackageAttribute>& list)
{
	for (int32 i = 0; i < value.CountStrings(); i++)
		AddStringAttribute(id, value.StringAt(i), list);
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
		WriteUnsignedLEB128(compose_attribute_tag(
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
