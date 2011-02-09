/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/RepositoryWriterImpl.h>

#include <algorithm>
#include <new>
#include <set>

#include <ByteOrder.h>
#include <Message.h>
#include <Path.h>

#include <package/hpkg/haiku_package.h>
#include <package/hpkg/PackageInfoAttributeValue.h>
#include <package/hpkg/PackageReader.h>
#include <package/ChecksumAccessors.h>
#include <package/RepositoryInfo.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


using BPackageKit::BPrivate::GeneralFileChecksumAccessor;


struct RepositoryWriterImpl::PackageNameSet : public std::set<BString> {
};


RepositoryWriterImpl::RepositoryWriterImpl(BRepositoryWriterListener* listener,
	const BRepositoryInfo* repositoryInfo)
	:
	WriterImplBase(listener),
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
RepositoryWriterImpl::HandleEntry(BPackageEntry* entry)
{
	return B_OK;
}


status_t
RepositoryWriterImpl::HandleEntryAttribute(BPackageEntry* entry,
	BPackageEntryAttribute* attribute)
{
	return B_OK;
}


status_t
RepositoryWriterImpl::HandleEntryDone(BPackageEntry* entry)
{
	return B_OK;
}


status_t
RepositoryWriterImpl::HandlePackageAttribute(
	const BPackageInfoAttributeValue& value)
{
	switch (value.attributeIndex) {
		case B_PACKAGE_INFO_NAME:
			fPackageInfo.SetName(value.string);
			break;

		case B_PACKAGE_INFO_SUMMARY:
			fPackageInfo.SetSummary(value.string);
			break;

		case B_PACKAGE_INFO_DESCRIPTION:
			fPackageInfo.SetDescription(value.string);
			break;

		case B_PACKAGE_INFO_VENDOR:
		{
			fPackageInfo.SetVendor(value.string);
			break;
		}

		case B_PACKAGE_INFO_PACKAGER:
			fPackageInfo.SetPackager(value.string);
			break;

		case B_PACKAGE_INFO_FLAGS:
			fPackageInfo.SetFlags(value.unsignedInt);
			break;

		case B_PACKAGE_INFO_ARCHITECTURE:
		{
			fPackageInfo.SetArchitecture(
				(BPackageArchitecture)value.unsignedInt);
			break;
		}

		case B_PACKAGE_INFO_VERSION:
			fPackageInfo.SetVersion(value.version);
			break;

		case B_PACKAGE_INFO_COPYRIGHTS:
			fPackageInfo.AddCopyright(value.string);
			break;

		case B_PACKAGE_INFO_LICENSES:
			fPackageInfo.AddLicense(value.string);
			break;

		case B_PACKAGE_INFO_PROVIDES:
			fPackageInfo.AddProvides(value.resolvable);
			break;

		case B_PACKAGE_INFO_REQUIRES:
			fPackageInfo.AddRequires(value.resolvableExpression);
			break;

		case B_PACKAGE_INFO_SUPPLEMENTS:
			fPackageInfo.AddSupplements(value.resolvableExpression);
			break;

		case B_PACKAGE_INFO_CONFLICTS:
			fPackageInfo.AddConflicts(value.resolvableExpression);
			break;

		case B_PACKAGE_INFO_FRESHENS:
			fPackageInfo.AddFreshens(value.resolvableExpression);
			break;

		case B_PACKAGE_INFO_REPLACES:
			fPackageInfo.AddReplaces(value.string);
			break;

		default:
			fListener->PrintError(
				"Invalid package attribute section: unexpected package "
				"attribute index %d encountered\n", value.attributeIndex);
			return B_BAD_DATA;
	}

	return B_OK;
}


status_t
RepositoryWriterImpl::HandlePackageAttributesDone()
{
	status_t result = _RegisterCurrentPackageInfo();
	fPackageInfo.Clear();

	return result;
}


void
RepositoryWriterImpl::HandleErrorOccurred()
{
}


status_t
RepositoryWriterImpl::_Init(const char* fileName)
{
	return WriterImplBase::Init(fileName, "repository");
}


status_t
RepositoryWriterImpl::_Finish()
{
	hpkg_repo_header header;

	// write repository header
	ssize_t repositoryInfoLength;
	status_t result = _WriteRepositoryInfo(repositoryInfoLength);
	if (result != B_OK)
		return result;

	header.repository_header_length
		= B_HOST_TO_BENDIAN_INT32(repositoryInfoLength);

	// write package attributes
	off_t totalSize = _WritePackageAttributes(header,
		sizeof(header) + repositoryInfoLength);

	fListener->OnRepositorySizeInfo(sizeof(header), repositoryInfoLength,
		fPackageCount,
		B_BENDIAN_TO_HOST_INT32(header.attributes_length_compressed),
		totalSize);

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

	GeneralFileChecksumAccessor checksumAccessor(packageEntry);
	BString checksum;
	if ((result = checksumAccessor.GetChecksum(checksum)) != B_OK) {
		fListener->PrintError("can't compute checksum!\n");
		return result;
	}
	fPackageInfo.SetChecksum(checksum);

	return packageReader.ParseContent(this);
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
	PackageNameSet::const_iterator namePos
		= fPackageNames->find(fPackageInfo.Name());
	if (namePos != fPackageNames->end()) {
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

	fPackageNames->insert(fPackageInfo.Name());

	RegisterPackageInfo(PackageAttributes(), fPackageInfo);
	fPackageCount++;
	fListener->OnPackageAdded(fPackageInfo);

	return B_OK;
}


status_t
RepositoryWriterImpl::_WriteRepositoryInfo(ssize_t& _repositoryInfoLength)
{
	BMessage archive;
	status_t result = fRepositoryInfo->Archive(&archive);
	if (result != B_OK) {
		fListener->PrintError("can't archive repository header!\n");
		return result;
	}

	_repositoryInfoLength = archive.FlattenedSize();
	char buffer[_repositoryInfoLength];
	if ((result = archive.Flatten(buffer, _repositoryInfoLength)) != B_OK) {
		fListener->PrintError("can't flatten repository header!\n");
		return result;
	}

	off_t startOffset = sizeof(hpkg_repo_header);
	WriteBuffer(buffer, _repositoryInfoLength, startOffset);

	return B_OK;
}

off_t
RepositoryWriterImpl::_WritePackageAttributes(hpkg_repo_header& header,
	off_t startOffset)
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

	fListener->OnPackageAttributesSizeInfo(stringsCount,
		zlibWriter.BytesWritten());

	// update the header
	header.attributes_compression
		= B_HOST_TO_BENDIAN_INT32(B_HPKG_COMPRESSION_ZLIB);
	header.attributes_length_compressed
		= B_HOST_TO_BENDIAN_INT32(endOffset - startOffset);
	header.attributes_length_uncompressed
		= B_HOST_TO_BENDIAN_INT32(zlibWriter.BytesWritten());
	header.attributes_strings_count = B_HOST_TO_BENDIAN_INT32(stringsCount);
	header.attributes_strings_length
		= B_HOST_TO_BENDIAN_INT32(stringsLengthUncompressed);

	return endOffset;
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
