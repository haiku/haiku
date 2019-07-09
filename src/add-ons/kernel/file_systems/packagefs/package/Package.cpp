/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Package.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <package/hpkg/ErrorOutput.h>
#include <package/hpkg/PackageDataReader.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>

#include <AutoDeleter.h>
#include <FdIO.h>
#include <package/hpkg/PackageFileHeapReader.h>
#include <package/hpkg/PackageReaderImpl.h>
#include <util/AutoLock.h>

#include "CachedDataReader.h"
#include "DebugSupport.h"
#include "PackageDirectory.h"
#include "PackageFile.h"
#include "PackagesDirectory.h"
#include "PackageSettings.h"
#include "PackageSymlink.h"
#include "Version.h"
#include "Volume.h"


using namespace BPackageKit;

using BPackageKit::BHPKG::BErrorOutput;
using BPackageKit::BHPKG::BFDDataReader;
using BPackageKit::BHPKG::BPackageInfoAttributeValue;
using BPackageKit::BHPKG::BPackageVersionData;
using BPackageKit::BHPKG::BPrivate::PackageFileHeapReader;

// current format version types
typedef BPackageKit::BHPKG::BPackageContentHandler BPackageContentHandler;
typedef BPackageKit::BHPKG::BPackageEntry BPackageEntry;
typedef BPackageKit::BHPKG::BPackageEntryAttribute BPackageEntryAttribute;
typedef BPackageKit::BHPKG::BPrivate::PackageReaderImpl PackageReaderImpl;


const char* const kArchitectureNames[B_PACKAGE_ARCHITECTURE_ENUM_COUNT] = {
	"any",
	"x86",
	"x86_gcc2",
	"source",
	"x86_64",
	"ppc",
	"arm",
	"m68k",
	"sparc",
	"arm64",
	"riscv64"
};


// #pragma mark - LoaderErrorOutput


struct Package::LoaderErrorOutput : BErrorOutput {
	LoaderErrorOutput(Package* package)
		:
		fPackage(package)
	{
	}

	virtual void PrintErrorVarArgs(const char* format, va_list args)
	{
		ERRORV(format, args);
	}

private:
	Package*	fPackage;
};


// #pragma mark - LoaderContentHandler


struct Package::LoaderContentHandler : BPackageContentHandler {
	LoaderContentHandler(Package* package, const PackageSettings& settings)
		:
		fPackage(package),
		fSettings(settings),
		fSettingsItem(NULL),
		fLastSettingsEntry(NULL),
		fLastSettingsEntryEntry(NULL),
		fErrorOccurred(false)
	{
	}

	status_t Init()
	{
		return B_OK;
	}

	virtual status_t HandleEntry(BPackageEntry* entry)
	{
		if (fErrorOccurred
			|| (fLastSettingsEntry != NULL
				&& fLastSettingsEntry->IsBlackListed())) {
			return B_OK;
		}

		PackageDirectory* parentDir = NULL;
		if (entry->Parent() != NULL) {
			parentDir = dynamic_cast<PackageDirectory*>(
				(PackageNode*)entry->Parent()->UserToken());
			if (parentDir == NULL)
				RETURN_ERROR(B_BAD_DATA);
		}

		if (fSettingsItem != NULL
			&& (parentDir == NULL
				|| entry->Parent() == fLastSettingsEntryEntry)) {
			PackageSettingsItem::Entry* settingsEntry
				= fSettingsItem->FindEntry(fLastSettingsEntry, entry->Name());
			if (settingsEntry != NULL) {
				fLastSettingsEntry = settingsEntry;
				fLastSettingsEntryEntry = entry;
				if (fLastSettingsEntry->IsBlackListed())
					return B_OK;
			}
		}

		// get the file mode -- filter out write permissions
		mode_t mode = entry->Mode() & ~(mode_t)(S_IWUSR | S_IWGRP | S_IWOTH);

		// create the package node
		PackageNode* node;
		if (S_ISREG(mode)) {
			// file
			node = new PackageFile(fPackage, mode,
				PackageData(entry->Data()));
		} else if (S_ISLNK(mode)) {
			// symlink
			String path;
			if (!path.SetTo(entry->SymlinkPath()))
				RETURN_ERROR(B_NO_MEMORY);

			PackageSymlink* symlink = new(std::nothrow) PackageSymlink(
				fPackage, mode);
			if (symlink == NULL)
				RETURN_ERROR(B_NO_MEMORY);

			symlink->SetSymlinkPath(path);
			node = symlink;
		} else if (S_ISDIR(mode)) {
			// directory
			node = new PackageDirectory(fPackage, mode);
		} else
			RETURN_ERROR(B_BAD_DATA);

		if (node == NULL)
			RETURN_ERROR(B_NO_MEMORY);
		BReference<PackageNode> nodeReference(node, true);

		String entryName;
		if (!entryName.SetTo(entry->Name()))
			RETURN_ERROR(B_NO_MEMORY);

		status_t error = node->Init(parentDir, entryName);
		if (error != B_OK)
			RETURN_ERROR(error);

		node->SetModifiedTime(entry->ModifiedTime());

		// add it to the parent directory
		if (parentDir != NULL)
			parentDir->AddChild(node);
		else
			fPackage->AddNode(node);

		entry->SetUserToken(node);

		return B_OK;
	}

	virtual status_t HandleEntryAttribute(BPackageEntry* entry,
		BPackageEntryAttribute* attribute)
	{
		if (fErrorOccurred
			|| (fLastSettingsEntry != NULL
				&& fLastSettingsEntry->IsBlackListed())) {
			return B_OK;
		}

		PackageNode* node = (PackageNode*)entry->UserToken();

		String name;
		if (!name.SetTo(attribute->Name()))
			RETURN_ERROR(B_NO_MEMORY);

		PackageNodeAttribute* nodeAttribute = new
			PackageNodeAttribute(attribute->Type(),
			PackageData(attribute->Data()));
		if (nodeAttribute == NULL)
			RETURN_ERROR(B_NO_MEMORY)

		nodeAttribute->Init(name);
		node->AddAttribute(nodeAttribute);

		return B_OK;
	}

	virtual status_t HandleEntryDone(BPackageEntry* entry)
	{
		if (entry == fLastSettingsEntryEntry) {
			fLastSettingsEntryEntry = entry->Parent();
			fLastSettingsEntry = fLastSettingsEntry->Parent();
		}

		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		switch (value.attributeID) {
			case B_PACKAGE_INFO_NAME:
			{
				String name;
				if (!name.SetTo(value.string))
					return B_NO_MEMORY;
				fPackage->SetName(name);

				fSettingsItem = fSettings.PackageItemFor(fPackage->Name());

				return B_OK;
			}

			case B_PACKAGE_INFO_INSTALL_PATH:
			{
				String path;
				if (!path.SetTo(value.string))
					return B_NO_MEMORY;
				fPackage->SetInstallPath(path);
				return B_OK;
			}

			case B_PACKAGE_INFO_VERSION:
			{
				::Version* version;
				status_t error = Version::Create(value.version.major,
					value.version.minor, value.version.micro,
					value.version.preRelease, value.version.revision, version);
				if (error != B_OK)
					RETURN_ERROR(error);

				fPackage->SetVersion(version);
				break;
			}

			case B_PACKAGE_INFO_FLAGS:
				fPackage->SetFlags(value.unsignedInt);
				break;

			case B_PACKAGE_INFO_ARCHITECTURE:
				if (value.unsignedInt >= B_PACKAGE_ARCHITECTURE_ENUM_COUNT)
					RETURN_ERROR(B_BAD_VALUE);

				fPackage->SetArchitecture(
					(BPackageArchitecture)value.unsignedInt);
				break;

			case B_PACKAGE_INFO_PROVIDES:
			{
				// create a version object, if a version is specified
				::Version* version = NULL;
				if (value.resolvable.haveVersion) {
					const BPackageVersionData& versionInfo
						= value.resolvable.version;
					status_t error = Version::Create(versionInfo.major,
						versionInfo.minor, versionInfo.micro,
						versionInfo.preRelease, versionInfo.revision, version);
					if (error != B_OK)
						RETURN_ERROR(error);
				}
				ObjectDeleter< ::Version> versionDeleter(version);

				// create a version object, if a compatible version is specified
				::Version* compatibleVersion = NULL;
				if (value.resolvable.haveCompatibleVersion) {
					const BPackageVersionData& versionInfo
						= value.resolvable.compatibleVersion;
					status_t error = Version::Create(versionInfo.major,
						versionInfo.minor, versionInfo.micro,
						versionInfo.preRelease, versionInfo.revision,
						compatibleVersion);
					if (error != B_OK)
						RETURN_ERROR(error);
				}
				ObjectDeleter< ::Version> compatibleVersionDeleter(
					compatibleVersion);

				// create the resolvable
				Resolvable* resolvable = new(std::nothrow) Resolvable(fPackage);
				if (resolvable == NULL)
					RETURN_ERROR(B_NO_MEMORY);
				ObjectDeleter<Resolvable> resolvableDeleter(resolvable);

				status_t error = resolvable->Init(value.resolvable.name,
					versionDeleter.Detach(), compatibleVersionDeleter.Detach());
				if (error != B_OK)
					RETURN_ERROR(error);

				fPackage->AddResolvable(resolvableDeleter.Detach());

				break;
			}

			case B_PACKAGE_INFO_REQUIRES:
			{
				// create the dependency
				Dependency* dependency = new(std::nothrow) Dependency(fPackage);
				if (dependency == NULL)
					RETURN_ERROR(B_NO_MEMORY);
				ObjectDeleter<Dependency> dependencyDeleter(dependency);

				status_t error = dependency->Init(
					value.resolvableExpression.name);
				if (error != B_OK)
					RETURN_ERROR(error);

				// create a version object, if a version is specified
				::Version* version = NULL;
				if (value.resolvableExpression.haveOpAndVersion) {
					const BPackageVersionData& versionInfo
						= value.resolvableExpression.version;
					status_t error = Version::Create(versionInfo.major,
						versionInfo.minor, versionInfo.micro,
						versionInfo.preRelease, versionInfo.revision, version);
					if (error != B_OK)
						RETURN_ERROR(error);

					dependency->SetVersionRequirement(
						value.resolvableExpression.op, version);
				}

				fPackage->AddDependency(dependencyDeleter.Detach());

				break;
			}

			default:
				break;
		}

		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
		fErrorOccurred = true;
	}

private:
	Package*					fPackage;
	const PackageSettings&		fSettings;
	const PackageSettingsItem*	fSettingsItem;
	PackageSettingsItem::Entry*	fLastSettingsEntry;
	const BPackageEntry*		fLastSettingsEntryEntry;
	bool						fErrorOccurred;
};


// #pragma mark - HeapReader


struct Package::HeapReader {
	virtual ~HeapReader()
	{
	}

	virtual void UpdateFD(int fd) = 0;

	virtual status_t CreateDataReader(const PackageData& data,
		BAbstractBufferedDataReader*& _reader) = 0;
};


// #pragma mark - HeapReaderV2


struct Package::HeapReaderV2 : public HeapReader, public CachedDataReader,
	private BErrorOutput, private BFdIO {
public:
	HeapReaderV2()
		:
		fHeapReader(NULL)
	{
	}

	~HeapReaderV2()
	{
		delete fHeapReader;
	}

	status_t Init(const PackageFileHeapReader* heapReader, int fd)
	{
		fHeapReader = heapReader->Clone();
		if (fHeapReader == NULL)
			return B_NO_MEMORY;

		BFdIO::SetTo(fd, false);

		fHeapReader->SetErrorOutput(this);
		fHeapReader->SetFile(this);

		status_t error = CachedDataReader::Init(fHeapReader,
			fHeapReader->UncompressedHeapSize());
		if (error != B_OK)
			return error;

		return B_OK;
	}

	virtual void UpdateFD(int fd)
	{
		BFdIO::SetTo(fd, false);
	}

	virtual status_t CreateDataReader(const PackageData& data,
		BAbstractBufferedDataReader*& _reader)
	{
		return BPackageKit::BHPKG::BPackageDataReaderFactory()
			.CreatePackageDataReader(this, data.DataV2(), _reader);
	}

private:
	// BErrorOutput

	virtual void PrintErrorVarArgs(const char* format, va_list args)
	{
		ERRORV(format, args);
	}

private:
	PackageFileHeapReader*	fHeapReader;
};


// #pragma mark - Package


struct Package::CachingPackageReader : public PackageReaderImpl {
	CachingPackageReader(BErrorOutput* errorOutput)
		:
		PackageReaderImpl(errorOutput),
		fCachedHeapReader(NULL),
		fFD(-1)
	{
	}

	~CachingPackageReader()
	{
	}

	status_t Init(int fd, bool keepFD, uint32 flags)
	{
		fFD = fd;
		return PackageReaderImpl::Init(fd, keepFD, flags);
	}

	virtual status_t CreateCachedHeapReader(
		PackageFileHeapReader* rawHeapReader,
		BAbstractBufferedDataReader*& _cachedReader)
	{
		fCachedHeapReader = new(std::nothrow) HeapReaderV2;
		if (fCachedHeapReader == NULL)
			RETURN_ERROR(B_NO_MEMORY);

		status_t error = fCachedHeapReader->Init(rawHeapReader, fFD);
		if (error != B_OK)
			RETURN_ERROR(error);

		_cachedReader = fCachedHeapReader;
		return B_OK;
	}

	HeapReaderV2* DetachCachedHeapReader()
	{
		PackageFileHeapReader* rawHeapReader;
		DetachHeapReader(rawHeapReader);

		// We don't need the raw heap reader anymore, since the cached reader
		// is not a wrapper around it, but completely independent from it.
		delete rawHeapReader;

		HeapReaderV2* cachedHeapReader = fCachedHeapReader;
		fCachedHeapReader = NULL;
		return cachedHeapReader;
	}

private:
	HeapReaderV2*	fCachedHeapReader;
	int				fFD;
};


// #pragma mark - Package


Package::Package(::Volume* volume, PackagesDirectory* directory, dev_t deviceID,
	ino_t nodeID)
	:
	fVolume(volume),
	fPackagesDirectory(directory),
	fFileName(),
	fName(),
	fInstallPath(),
	fVersionedName(),
	fVersion(NULL),
	fFlags(0),
	fArchitecture(B_PACKAGE_ARCHITECTURE_ENUM_COUNT),
	fLinkDirectory(NULL),
	fFD(-1),
	fOpenCount(0),
	fHeapReader(NULL),
	fNodeID(nodeID),
	fDeviceID(deviceID)
{
	mutex_init(&fLock, "packagefs package");

	fPackagesDirectory->AcquireReference();
}


Package::~Package()
{
	delete fHeapReader;

	while (PackageNode* node = fNodes.RemoveHead())
		node->ReleaseReference();

	while (Resolvable* resolvable = fResolvables.RemoveHead())
		delete resolvable;

	while (Dependency* dependency = fDependencies.RemoveHead())
		delete dependency;

	delete fVersion;

	fPackagesDirectory->ReleaseReference();

	mutex_destroy(&fLock);
}


status_t
Package::Init(const char* fileName)
{
	if (!fFileName.SetTo(fileName))
		RETURN_ERROR(B_NO_MEMORY);

	return B_OK;
}


status_t
Package::Load(const PackageSettings& settings)
{
	status_t error = _Load(settings);
	if (error != B_OK)
		return error;

	if (!_InitVersionedName())
		RETURN_ERROR(B_NO_MEMORY);

	return B_OK;
}


void
Package::SetName(const String& name)
{
	fName = name;
}


void
Package::SetInstallPath(const String& installPath)
{
	fInstallPath = installPath;
}


void
Package::SetVersion(::Version* version)
{
	if (fVersion != NULL)
		delete fVersion;

	fVersion = version;
}


const char*
Package::ArchitectureName() const
{
	if (fArchitecture < 0
		|| fArchitecture >= B_PACKAGE_ARCHITECTURE_ENUM_COUNT) {
		return NULL;
	}

	return kArchitectureNames[fArchitecture];
}


void
Package::AddNode(PackageNode* node)
{
	fNodes.Add(node);
	node->AcquireReference();
}


void
Package::AddResolvable(Resolvable* resolvable)
{
	fResolvables.Add(resolvable);
}


void
Package::AddDependency(Dependency* dependency)
{
	fDependencies.Add(dependency);
}


int
Package::Open()
{
	MutexLocker locker(fLock);
	if (fOpenCount > 0) {
		fOpenCount++;
		return fFD;
	}

	// open the file
	fFD = openat(fPackagesDirectory->DirectoryFD(), fFileName,
		O_RDONLY | O_NOCACHE);
	if (fFD < 0) {
		ERROR("Failed to open package file \"%s\": %s\n", fFileName.Data(),
			strerror(errno));
		return errno;
	}

	// stat it to verify that it's still the same file
	struct stat st;
	if (fstat(fFD, &st) < 0) {
		ERROR("Failed to stat package file \"%s\": %s\n", fFileName.Data(),
			strerror(errno));
		close(fFD);
		fFD = -1;
		return errno;
	}

	if (st.st_dev != fDeviceID || st.st_ino != fNodeID) {
		close(fFD);
		fFD = -1;
		RETURN_ERROR(B_ENTRY_NOT_FOUND);
	}

	fOpenCount = 1;

	if (fHeapReader != NULL)
		fHeapReader->UpdateFD(fFD);

	return fFD;
}


void
Package::Close()
{
	MutexLocker locker(fLock);
	if (fOpenCount == 0) {
		ERROR("Package open count already 0!\n");
		return;
	}

	if (--fOpenCount == 0) {
		close(fFD);
		fFD = -1;

		if (fHeapReader != NULL)
			fHeapReader->UpdateFD(fFD);
	}
}


status_t
Package::CreateDataReader(const PackageData& data,
	BAbstractBufferedDataReader*& _reader)
{
	if (fHeapReader == NULL)
		return B_BAD_VALUE;

	return fHeapReader->CreateDataReader(data, _reader);
}


status_t
Package::_Load(const PackageSettings& settings)
{
	// open package file
	int fd = Open();
	if (fd < 0)
		RETURN_ERROR(fd);
	PackageCloser packageCloser(this);

	// initialize package reader
	LoaderErrorOutput errorOutput(this);

	// try current package file format version
	{
		CachingPackageReader packageReader(&errorOutput);
		status_t error = packageReader.Init(fd, false,
			BHPKG::B_HPKG_READER_DONT_PRINT_VERSION_MISMATCH_MESSAGE);
		if (error == B_OK) {
			// parse content
			LoaderContentHandler handler(this, settings);
			error = handler.Init();
			if (error != B_OK)
				RETURN_ERROR(error);

			error = packageReader.ParseContent(&handler);
			if (error != B_OK)
				RETURN_ERROR(error);

			// get the heap reader
			fHeapReader = packageReader.DetachCachedHeapReader();
			return B_OK;
		}

		if (error != B_MISMATCHED_VALUES)
			RETURN_ERROR(error);
	}

	// we don't support this package file format
	RETURN_ERROR(B_BAD_DATA);
}


bool
Package::_InitVersionedName()
{
	// compute the allocation size needed for the versioned name
	size_t nameLength = strlen(fName);
	size_t size = nameLength + 1;

	if (fVersion != NULL) {
		size += 1 + fVersion->ToString(NULL, 0);
			// + 1 for the '-'
	}

	// allocate the name and compose it
	char* name = (char*)malloc(size);
	if (name == NULL)
		return false;
	MemoryDeleter nameDeleter(name);

	memcpy(name, fName, nameLength + 1);
	if (fVersion != NULL) {
		name[nameLength] = '-';
		fVersion->ToString(name + nameLength + 1, size - nameLength - 1);
	}

	return fVersionedName.SetTo(name);
}
