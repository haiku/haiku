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

#include <package/hpkg/BlockBufferPoolNoLock.h>
#include <package/hpkg/HPKGDefsPrivate.h>
#include <package/hpkg/PackageDataReader.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageFileHeapWriter.h>
#include <package/hpkg/PackageInfoAttributeValue.h>
#include <package/hpkg/PackageReader.h>
#include <package/ChecksumAccessors.h>
#include <package/HashableString.h>
#include <package/PackageInfoContentHandler.h>
#include <package/RepositoryInfo.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


using BPackageKit::BPrivate::GeneralFileChecksumAccessor;
using BPackageKit::BPrivate::HashableString;


namespace {


// #pragma mark - PackageEntryDataFetcher


struct PackageEntryDataFetcher {
	PackageEntryDataFetcher(BErrorOutput* errorOutput,
		BPackageData& packageData)
		:
		fErrorOutput(errorOutput),
		fPackageData(packageData)
	{
	}

	status_t ReadIntoString(BAbstractBufferedDataReader* heapReader,
		BString& _contents)
	{
		// create a PackageDataReader
		BAbstractBufferedDataReader* reader;
		status_t result = BPackageDataReaderFactory()
			.CreatePackageDataReader(heapReader, fPackageData, reader);
		if (result != B_OK)
			return result;
		ObjectDeleter<BAbstractBufferedDataReader> readerDeleter(reader);

		// copy data into the given string
		int32 bufferSize = fPackageData.Size();
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
	BPackageData&			fPackageData;
};


// #pragma mark - PackageContentHandler


struct PackageContentHandler : public BPackageInfoContentHandler {
	PackageContentHandler(BErrorOutput* errorOutput, BPackageInfo* packageInfo,
		BAbstractBufferedDataReader* heapReader,
		BRepositoryInfo* repositoryInfo)
		:
		BPackageInfoContentHandler(*packageInfo, errorOutput),
		fHeapReader(heapReader),
		fRepositoryInfo(repositoryInfo)
	{
	}

	virtual status_t HandleEntry(BPackageEntry* entry)
	{
		// if license must be approved, read any license files from package such
		// that those can be stored in the repository later
		if ((fPackageInfo.Flags() & B_PACKAGE_FLAG_APPROVE_LICENSE) == 0
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
		status_t result = dataFetcher.ReadIntoString(fHeapReader, licenseText);
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

	virtual void HandleErrorOccurred()
	{
	}

private:
	BPackageReader*					fPackageReader;
	BAbstractBufferedDataReader*	fHeapReader;
	BRepositoryInfo*				fRepositoryInfo;
};


}	// anonymous namespace


// #pragma mark - PackageNameSet


struct RepositoryWriterImpl::PackageNameSet
	: public ::BPrivate::HashSet<HashableString> {
};


// #pragma mark - RepositoryWriterImpl


RepositoryWriterImpl::RepositoryWriterImpl(BRepositoryWriterListener* listener,
	BRepositoryInfo* repositoryInfo)
	:
	inherited("repository", listener),
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
RepositoryWriterImpl::AddPackageInfo(const BPackageInfo& packageInfo)
{
	try {
		return _AddPackageInfo(packageInfo);
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
	status_t error = inherited::Init(NULL, false, fileName,
		BPackageWriterParameters());
	if (error != B_OK)
		return error;

	return InitHeapReader(sizeof(hpkg_repo_header));
}


status_t
RepositoryWriterImpl::_Finish()
{
	hpkg_repo_header header;

	// write repository info
	uint64 infoLength;
	status_t result = _WriteRepositoryInfo(header, infoLength);
	if (result != B_OK)
		return result;

	// write package attributes
	uint64 packagesLength;
	_WritePackageAttributes(header, packagesLength);

	// flush the heap writer
	result = fHeapWriter->Finish();
	if (result != B_OK)
		return result;
	uint64 compressedHeapSize = fHeapWriter->CompressedHeapSize();
	uint64 totalSize = fHeapWriter->HeapOffset() + compressedHeapSize;

	header.heap_compression = B_HOST_TO_BENDIAN_INT16(
		Parameters().Compression());
	header.heap_chunk_size = B_HOST_TO_BENDIAN_INT32(fHeapWriter->ChunkSize());
	header.heap_size_compressed = B_HOST_TO_BENDIAN_INT64(compressedHeapSize);
	header.heap_size_uncompressed = B_HOST_TO_BENDIAN_INT64(
		fHeapWriter->UncompressedHeapSize());

	fListener->OnRepositoryDone(sizeof(header), infoLength,
		fRepositoryInfo->LicenseNames().CountStrings(), fPackageCount,
		packagesLength, totalSize);

	// update the general header info and write the header
	header.magic = B_HOST_TO_BENDIAN_INT32(B_HPKG_REPO_MAGIC);
	header.header_size = B_HOST_TO_BENDIAN_INT16((uint16)sizeof(header));
	header.version = B_HOST_TO_BENDIAN_INT16(B_HPKG_REPO_VERSION);
	header.total_size = B_HOST_TO_BENDIAN_INT64(totalSize);
	header.minor_version = B_HOST_TO_BENDIAN_INT16(B_HPKG_REPO_MINOR_VERSION);

	RawWriteBuffer(&header, sizeof(header), 0);

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
		fListener->PrintError("can't get path for entry '%s'!\n",
			packageEntry.Name());
		return result;
	}

	BPackageReader packageReader(fListener);
	if ((result = packageReader.Init(packagePath.Path())) != B_OK) {
		fListener->PrintError("can't create package reader for '%s'!\n",
			packagePath.Path());
		return result;
	}

	fPackageInfo.Clear();

	// parse package
	PackageContentHandler contentHandler(fListener, &fPackageInfo,
		packageReader.HeapReader(), fRepositoryInfo);
	if ((result = packageReader.ParseContent(&contentHandler)) != B_OK)
		return result;

	// determine package's checksum
	GeneralFileChecksumAccessor checksumAccessor(packageEntry);
	BString checksum;
	if ((result = checksumAccessor.GetChecksum(checksum)) != B_OK) {
		fListener->PrintError("can't compute checksum of file '%s'!\n",
			packagePath.Path());
		return result;
	}
	fPackageInfo.SetChecksum(checksum);

	// register package's attributes
	if ((result = _RegisterCurrentPackageInfo()) != B_OK)
		return result;

	return B_OK;
}


status_t
RepositoryWriterImpl::_AddPackageInfo(const BPackageInfo& packageInfo)
{
	fPackageInfo = packageInfo;

	// register package's attributes
	status_t result = _RegisterCurrentPackageInfo();
	if (result != B_OK)
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
		&& fPackageInfo.Architecture() != B_PACKAGE_ARCHITECTURE_ANY
		&& fPackageInfo.Architecture() != B_PACKAGE_ARCHITECTURE_SOURCE) {
		fListener->PrintError(
			"package '%s' has non-matching architecture '%s' "
			"(expected '%s', '%s', or '%s')!\n", fPackageInfo.Name().String(),
			BPackageInfo::kArchitectureNames[fPackageInfo.Architecture()],
			BPackageInfo::kArchitectureNames[expectedArchitecture],
			BPackageInfo::kArchitectureNames[B_PACKAGE_ARCHITECTURE_ANY],
			BPackageInfo::kArchitectureNames[B_PACKAGE_ARCHITECTURE_SOURCE]);
		return B_BAD_DATA;
	}

	if ((result = fPackageNames->Add(fPackageInfo.Name())) != B_OK)
		return result;

	PackageAttribute* packageAttribute = AddStringAttribute(
		B_HPKG_ATTRIBUTE_ID_PACKAGE, fPackageInfo.Name(), PackageAttributes());
	RegisterPackageInfo(packageAttribute->children, fPackageInfo);
	fPackageCount++;
	fListener->OnPackageAdded(fPackageInfo);

	return B_OK;
}


status_t
RepositoryWriterImpl::_WriteRepositoryInfo(hpkg_repo_header& header,
	uint64& _length)
{
	// archive and flatten the repository info and write it
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

	WriteBuffer(buffer, flattenedSize);

	// notify listener
	fListener->OnRepositoryInfoSectionDone(flattenedSize);

	// update the header
	header.info_length = B_HOST_TO_BENDIAN_INT32(flattenedSize);

	_length = flattenedSize;
	return B_OK;
}


void
RepositoryWriterImpl::_WritePackageAttributes(hpkg_repo_header& header,
	uint64& _length)
{
	// write the package attributes (zlib writer on top of a file writer)
	uint64 startOffset = fHeapWriter->UncompressedHeapSize();

	uint32 stringsLength;
	uint32 stringsCount = WritePackageAttributes(PackageAttributes(),
		stringsLength);

	uint64 sectionSize = fHeapWriter->UncompressedHeapSize() - startOffset;

	fListener->OnPackageAttributesSectionDone(stringsCount, sectionSize);

	// update the header
	header.packages_length = B_HOST_TO_BENDIAN_INT64(sectionSize);
	header.packages_strings_count = B_HOST_TO_BENDIAN_INT64(stringsCount);
	header.packages_strings_length = B_HOST_TO_BENDIAN_INT64(stringsLength);

	_length = sectionSize;
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
