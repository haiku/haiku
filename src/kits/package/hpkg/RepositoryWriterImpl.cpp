/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/RepositoryWriterImpl.h>

#include <algorithm>
#include <new>

#include <ByteOrder.h>
#include <Message.h>
#include <Path.h>

#include <AutoDeleter.h>
#include <HashSet.h>

#include <package/hpkg/HPKGDefsPrivate.h>
#include <package/hpkg/PackageDataReader.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageInfoAttributeValue.h>
#include <package/hpkg/PackageReader.h>
#include <package/BlockBufferCacheNoLock.h>
#include <package/ChecksumAccessors.h>
#include <package/HashableString.h>
#include <package/RepositoryInfo.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


using BPackageKit::BPrivate::GeneralFileChecksumAccessor;
using BPackageKit::BPrivate::HashableString;


namespace {


struct PackageEntryDataFetcher {
	PackageEntryDataFetcher(BErrorOutput* errorOutput,
		BPackageData& packageData)
		:
		fErrorOutput(errorOutput),
		fBufferCache(B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB, 2),
		fPackageData(packageData)
	{
	}

	status_t ReadIntoString(BDataReader* dataReader, BString& _contents)
	{
		// create a BPackageDataReader
		BPackageDataReader* reader;
		status_t result = BPackageDataReaderFactory(&fBufferCache)
			.CreatePackageDataReader(dataReader, fPackageData, reader);
		if (result != B_OK)
			return result;
		ObjectDeleter<BPackageDataReader> readerDeleter(reader);

		// copy data into the given string
		int32 bufferSize = fPackageData.UncompressedSize();
		char* buffer = _contents.LockBuffer(bufferSize);
		if (buffer == NULL)
			return B_NO_MEMORY;

		result = reader->ReadData(0, buffer, bufferSize);
		if (result != B_OK) {
			fErrorOutput->PrintError("Error: Failed to read data: %s\n",
				strerror(result));
			_contents.UnlockBuffer(0);
		} else
			_contents.UnlockBuffer(bufferSize);

		return result;
	}

private:
	BErrorOutput*			fErrorOutput;
	BBlockBufferCacheNoLock fBufferCache;
	BPackageData&			fPackageData;
};


struct PackageContentHandler : public BPackageContentHandler {
	PackageContentHandler(BErrorOutput* errorOutput, BPackageInfo* packageInfo,
		int packageFileFD, BRepositoryInfo* repositoryInfo)
		:
		fErrorOutput(errorOutput),
		fPackageInfo(packageInfo),
		fPackageFileReader(packageFileFD),
		fRepositoryInfo(repositoryInfo)
	{
	}

	virtual status_t HandleEntry(BPackageEntry* entry)
	{
		// if license must be approved, read any license files from package such
		// that those can be stored in the repository later
		if ((fPackageInfo->Flags() & B_PACKAGE_FLAG_APPROVE_LICENSE) == 0
			|| entry == NULL)
			return B_OK;

		// return if not in ./data/licenses folder
		const BPackageEntry* parent = entry->Parent();
		BString licenseFolderName("licenses");
		if (parent == NULL || licenseFolderName != parent->Name())
			return B_OK;

		parent = parent->Parent();
		BString dataFolderName("data");
		if (parent == NULL || dataFolderName != parent->Name())
			return B_OK;

		if (parent->Parent() != NULL)
			return B_OK;

		// check if license already is in repository
		const BStringList& licenseNames = fRepositoryInfo->LicenseNames();
		for (int i = 0; i < licenseNames.CountStrings(); ++i) {
			if (licenseNames.StringAt(i).ICompare(entry->Name()) == 0) {
				// license already exists
				return B_OK;
			}
		}

		// fetch contents of license file
		BPackageData& packageData = entry->Data();
		PackageEntryDataFetcher dataFetcher(fErrorOutput, packageData);

		BString licenseText;
		status_t result;
		if (packageData.IsEncodedInline()) {
			BBufferDataReader dataReader(packageData.InlineData(),
				packageData.CompressedSize());
			result = dataFetcher.ReadIntoString(&dataReader, licenseText);
		} else {
			result
				= dataFetcher.ReadIntoString(&fPackageFileReader, licenseText);
		}

		if (result != B_OK)
			return result;

		// add license to repository
		return fRepositoryInfo->AddLicense(entry->Name(), licenseText);
	}

	virtual status_t HandleEntryAttribute(BPackageEntry* entry,
		BPackageEntryAttribute* attribute)
	{
		return B_OK;
	}

	virtual status_t HandleEntryDone(BPackageEntry* entry)
	{
		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		switch (value.attributeID) {
			case B_PACKAGE_INFO_NAME:
				fPackageInfo->SetName(value.string);
				break;

			case B_PACKAGE_INFO_SUMMARY:
				fPackageInfo->SetSummary(value.string);
				break;

			case B_PACKAGE_INFO_DESCRIPTION:
				fPackageInfo->SetDescription(value.string);
				break;

			case B_PACKAGE_INFO_VENDOR:
				fPackageInfo->SetVendor(value.string);
				break;

			case B_PACKAGE_INFO_PACKAGER:
				fPackageInfo->SetPackager(value.string);
				break;

			case B_PACKAGE_INFO_FLAGS:
				fPackageInfo->SetFlags(value.unsignedInt);
				break;

			case B_PACKAGE_INFO_ARCHITECTURE:
				fPackageInfo->SetArchitecture(
					(BPackageArchitecture)value.unsignedInt);
				break;

			case B_PACKAGE_INFO_VERSION:
				fPackageInfo->SetVersion(value.version);
				break;

			case B_PACKAGE_INFO_COPYRIGHTS:
				fPackageInfo->AddCopyright(value.string);
				break;

			case B_PACKAGE_INFO_LICENSES:
				fPackageInfo->AddLicense(value.string);
				break;

			case B_PACKAGE_INFO_PROVIDES:
				fPackageInfo->AddProvides(value.resolvable);
				break;

			case B_PACKAGE_INFO_REQUIRES:
				fPackageInfo->AddRequires(value.resolvableExpression);
				break;

			case B_PACKAGE_INFO_SUPPLEMENTS:
				fPackageInfo->AddSupplements(value.resolvableExpression);
				break;

			case B_PACKAGE_INFO_CONFLICTS:
				fPackageInfo->AddConflicts(value.resolvableExpression);
				break;

			case B_PACKAGE_INFO_FRESHENS:
				fPackageInfo->AddFreshens(value.resolvableExpression);
				break;

			case B_PACKAGE_INFO_REPLACES:
				fPackageInfo->AddReplaces(value.string);
				break;

			case B_PACKAGE_INFO_URLS:
				fPackageInfo->AddURL(value.string);
				break;

			case B_PACKAGE_INFO_SOURCE_URLS:
				fPackageInfo->AddSourceURL(value.string);
				break;

			case B_PACKAGE_INFO_CHECKSUM:
				fPackageInfo->SetChecksum(value.string);
				break;

			case B_PACKAGE_INFO_INSTALL_PATH:
				fPackageInfo->SetInstallPath(value.string);
				break;

			default:
				fErrorOutput->PrintError(
					"Invalid package attribute section: unexpected package "
					"attribute id %d encountered\n", value.attributeID);
				return B_BAD_DATA;
		}

		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
	}

private:
	BErrorOutput* 		fErrorOutput;
	BPackageInfo* 		fPackageInfo;
	BPackageReader*		fPackageReader;
	BFDDataReader		fPackageFileReader;
	BRepositoryInfo*	fRepositoryInfo;
};


}	// anonymous namespace


struct RepositoryWriterImpl::PackageNameSet
	: public ::BPrivate::HashSet<HashableString> {
};


RepositoryWriterImpl::RepositoryWriterImpl(BRepositoryWriterListener* listener,
	BRepositoryInfo* repositoryInfo)
	:
	inherited(listener),
	fListener(listener),
	fRepositoryInfo(repositoryInfo),
	fPackageCount(0),
	fPackageNames(NULL)
{
}


RepositoryWriterImpl::~RepositoryWriterImpl()
{
	delete fPackageNames;
}


status_t
RepositoryWriterImpl::Init(const char* fileName)
{
	try {
		fPackageNames = new PackageNameSet();
		status_t result = fPackageNames->InitCheck();
		if (result != B_OK)
			return result;
		return _Init(fileName);
	} catch (status_t error) {
		return error;
	} catch (std::bad_alloc) {
		fListener->PrintError("Out of memory!\n");
		return B_NO_MEMORY;
	}
}


status_t
RepositoryWriterImpl::AddPackage(const BEntry& packageEntry)
{
	try {
		return _AddPackage(packageEntry);
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
	return inherited::Init(fileName, "repository", 0);
}


status_t
RepositoryWriterImpl::_Finish()
{
	hpkg_repo_header header;

	// write repository header
	ssize_t infoLengthCompressed;
	status_t result = _WriteRepositoryInfo(header, infoLengthCompressed);
	if (result != B_OK)
		return result;

	// write package attributes
	ssize_t packagesLengthCompressed;
	off_t totalSize = _WritePackageAttributes(header,
		sizeof(header) + infoLengthCompressed, packagesLengthCompressed);

	fListener->OnRepositoryDone(sizeof(header), infoLengthCompressed,
		fRepositoryInfo->LicenseNames().CountStrings(), fPackageCount,
		packagesLengthCompressed, totalSize);

	// general
	header.magic = B_HOST_TO_BENDIAN_INT32(B_HPKG_REPO_MAGIC);
	header.header_size = B_HOST_TO_BENDIAN_INT16((uint16)sizeof(header));
	header.version = B_HOST_TO_BENDIAN_INT16(B_HPKG_REPO_VERSION);
	header.total_size = B_HOST_TO_BENDIAN_INT64(totalSize);

	// write the header
	WriteBuffer(&header, sizeof(header), 0);

	SetFinished(true);
	return B_OK;
}


status_t
RepositoryWriterImpl::_AddPackage(const BEntry& packageEntry)
{
	status_t result = packageEntry.InitCheck();
	if (result != B_OK) {
		fListener->PrintError("entry not initialized!\n");
		return result;
	}

	BPath packagePath;
	if ((result = packageEntry.GetPath(&packagePath)) != B_OK) {
		fListener->PrintError("can't get path for entry!\n");
		return result;
	}

	BPackageReader packageReader(fListener);
	if ((result = packageReader.Init(packagePath.Path())) != B_OK) {
		fListener->PrintError("can't create package reader!\n");
		return result;
	}

	fPackageInfo.Clear();

	// parse package
	PackageContentHandler contentHandler(fListener, &fPackageInfo,
		packageReader.PackageFileFD(), fRepositoryInfo);
	if ((result = packageReader.ParseContent(&contentHandler)) != B_OK)
		return result;

	// determine package's checksum
	GeneralFileChecksumAccessor checksumAccessor(packageEntry);
	BString checksum;
	if ((result = checksumAccessor.GetChecksum(checksum)) != B_OK) {
		fListener->PrintError("can't compute checksum!\n");
		return result;
	}
	fPackageInfo.SetChecksum(checksum);

	// register package's attributes
	if ((result = _RegisterCurrentPackageInfo()) != B_OK)
		return result;

	return B_OK;
}


status_t
RepositoryWriterImpl::_RegisterCurrentPackageInfo()
{
	status_t result = fPackageInfo.InitCheck();
	if (result != B_OK) {
		fListener->PrintError("package %s has incomplete package-info!\n",
			fPackageInfo.Name().String());
		return result;
	}

	// reject package with a name that we've seen already
	if (fPackageNames->Contains(fPackageInfo.Name())) {
		fListener->PrintError("package %s has already been added!\n",
			fPackageInfo.Name().String());
		return B_NAME_IN_USE;
	}

	// all packages must have the same vendor as the repository
	const BString& expectedVendor = fRepositoryInfo->Vendor();
	if (fPackageInfo.Vendor().ICompare(expectedVendor) != 0) {
		fListener->PrintError("package '%s' has unexpected vendor '%s' "
			"(expected '%s')!\n", fPackageInfo.Name().String(),
			fPackageInfo.Vendor().String(), expectedVendor.String());
		return B_BAD_DATA;
	}

	// all packages must have an architecture that's compatible with the one
	// used by the repository
	BPackageArchitecture expectedArchitecture = fRepositoryInfo->Architecture();
	if (fPackageInfo.Architecture() != expectedArchitecture
		&& fPackageInfo.Architecture() != B_PACKAGE_ARCHITECTURE_ANY) {
		fListener->PrintError(
			"package '%s' has non-matching architecture '%s' "
			"(expected '%s' or '%s')!\n", fPackageInfo.Name().String(),
			BPackageInfo::kArchitectureNames[fPackageInfo.Architecture()],
			BPackageInfo::kArchitectureNames[expectedArchitecture],
			BPackageInfo::kArchitectureNames[B_PACKAGE_ARCHITECTURE_ANY]
		);
		return B_BAD_DATA;
	}

	if ((result = fPackageNames->Add(fPackageInfo.Name())) != B_OK)
		return result;

	RegisterPackageInfo(PackageAttributes(), fPackageInfo);
	fPackageCount++;
	fListener->OnPackageAdded(fPackageInfo);

	return B_OK;
}


status_t
RepositoryWriterImpl::_WriteRepositoryInfo(hpkg_repo_header& header,
	ssize_t& _infoLengthCompressed)
{
	BMessage archive;
	status_t result = fRepositoryInfo->Archive(&archive);
	if (result != B_OK) {
		fListener->PrintError("can't archive repository header!\n");
		return result;
	}

	ssize_t	flattenedSize = archive.FlattenedSize();
	char buffer[flattenedSize];
	if ((result = archive.Flatten(buffer, flattenedSize)) != B_OK) {
		fListener->PrintError("can't flatten repository header!\n");
		return result;
	}

	off_t startOffset = sizeof(hpkg_repo_header);

	// write the package attributes (zlib writer on top of a file writer)
	FDDataWriter realWriter(FD(), startOffset, fListener);
	ZlibDataWriter zlibWriter(&realWriter);
	SetDataWriter(&zlibWriter);
	zlibWriter.Init();

	DataWriter()->WriteDataThrows(buffer, flattenedSize);

	zlibWriter.Finish();
	SetDataWriter(NULL);

	fListener->OnRepositoryInfoSectionDone(zlibWriter.BytesWritten());

	_infoLengthCompressed = realWriter.BytesWritten();

	// update the header
	header.info_compression
		= B_HOST_TO_BENDIAN_INT32(B_HPKG_COMPRESSION_ZLIB);
	header.info_length_compressed
		= B_HOST_TO_BENDIAN_INT32(_infoLengthCompressed);
	header.info_length_uncompressed
		= B_HOST_TO_BENDIAN_INT32(flattenedSize);

	return B_OK;
}


off_t
RepositoryWriterImpl::_WritePackageAttributes(hpkg_repo_header& header,
	off_t startOffset, ssize_t& _packagesLengthCompressed)
{
	// write the package attributes (zlib writer on top of a file writer)
	FDDataWriter realWriter(FD(), startOffset, fListener);
	ZlibDataWriter zlibWriter(&realWriter);
	SetDataWriter(&zlibWriter);
	zlibWriter.Init();

	// write cached strings and package attributes tree
	uint32 stringsLengthUncompressed;
	uint32 stringsCount = WritePackageAttributes(PackageAttributes(),
		stringsLengthUncompressed);

	zlibWriter.Finish();
	off_t endOffset = realWriter.Offset();
	SetDataWriter(NULL);

	fListener->OnPackageAttributesSectionDone(stringsCount,
		zlibWriter.BytesWritten());

	_packagesLengthCompressed = endOffset - startOffset;

	// update the header
	header.packages_compression
		= B_HOST_TO_BENDIAN_INT32(B_HPKG_COMPRESSION_ZLIB);
	header.packages_length_compressed
		= B_HOST_TO_BENDIAN_INT64(_packagesLengthCompressed);
	header.packages_length_uncompressed
		= B_HOST_TO_BENDIAN_INT64(zlibWriter.BytesWritten());
	header.packages_strings_count = B_HOST_TO_BENDIAN_INT64(stringsCount);
	header.packages_strings_length
		= B_HOST_TO_BENDIAN_INT64(stringsLengthUncompressed);

	return endOffset;
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
