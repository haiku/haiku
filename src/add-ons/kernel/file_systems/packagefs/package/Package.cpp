/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Package.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <AutoDeleter.h>
#include <package/hpkg/ErrorOutput.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>
#include <package/hpkg/PackageReaderImpl.h>
#include <util/AutoLock.h>

#include "DebugSupport.h"
#include "PackageDirectory.h"
#include "PackageFile.h"
#include "PackageSymlink.h"
#include "Version.h"
#include "Volume.h"


using namespace BPackageKit;
using namespace BPackageKit::BHPKG;
using BPackageKit::BHPKG::BPrivate::PackageReaderImpl;


const char* const kArchitectureNames[B_PACKAGE_ARCHITECTURE_ENUM_COUNT] = {
	"any",
	"x86",
	"x86_gcc2",
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
// TODO:...
	}

private:
	Package*	fPackage;
};


// #pragma mark - LoaderContentHandler


struct Package::LoaderContentHandler : BPackageContentHandler {
	LoaderContentHandler(Package* package)
		:
		fPackage(package),
		fErrorOccurred(false)
	{
	}

	status_t Init()
	{
		return B_OK;
	}

	virtual status_t HandleEntry(BPackageEntry* entry)
	{
		if (fErrorOccurred)
			return B_OK;

		PackageDirectory* parentDir = NULL;
		if (entry->Parent() != NULL) {
			parentDir = dynamic_cast<PackageDirectory*>(
				(PackageNode*)entry->Parent()->UserToken());
			if (parentDir == NULL)
				RETURN_ERROR(B_BAD_DATA);
		}

		// get the file mode -- filter out write permissions
		mode_t mode = entry->Mode() & ~(mode_t)(S_IWUSR | S_IWGRP | S_IWOTH);

		// create the package node
		PackageNode* node;
		if (S_ISREG(mode)) {
			// file
			node = new(std::nothrow) PackageFile(fPackage, mode, entry->Data());
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
			node = new(std::nothrow) PackageDirectory(fPackage, mode);
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
		if (fErrorOccurred)
			return B_OK;

		PackageNode* node = (PackageNode*)entry->UserToken();

		String name;
		if (!name.SetTo(attribute->Name()))
			RETURN_ERROR(B_NO_MEMORY);

		PackageNodeAttribute* nodeAttribute = new(std::nothrow)
			PackageNodeAttribute(attribute->Type(), attribute->Data());
		if (nodeAttribute == NULL)
			RETURN_ERROR(B_NO_MEMORY)

		nodeAttribute->Init(name);
		node->AddAttribute(nodeAttribute);

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
			{
				String name;
				if (!name.SetTo(value.string))
					return B_NO_MEMORY;
				fPackage->SetName(name);
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
	Package*	fPackage;
	bool		fErrorOccurred;
};


// #pragma mark - Package


Package::Package(::Volume* volume, dev_t deviceID, ino_t nodeID)
	:
	fVolume(volume),
	fFileName(),
	fName(),
	fInstallPath(),
	fVersion(NULL),
	fArchitecture(B_PACKAGE_ARCHITECTURE_ENUM_COUNT),
	fLinkDirectory(NULL),
	fFD(-1),
	fOpenCount(0),
	fNodeID(nodeID),
	fDeviceID(deviceID)
{
	mutex_init(&fLock, "packagefs package");
}


Package::~Package()
{
	while (PackageNode* node = fNodes.RemoveHead())
		node->ReleaseReference();

	while (Resolvable* resolvable = fResolvables.RemoveHead())
		delete resolvable;

	while (Dependency* dependency = fDependencies.RemoveHead())
		delete dependency;

	delete fVersion;

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
Package::Load()
{
	// open package file
	int fd = Open();
	if (fd < 0)
		RETURN_ERROR(fd);
	PackageCloser packageCloser(this);

	// initialize package reader
	LoaderErrorOutput errorOutput(this);
	PackageReaderImpl packageReader(&errorOutput);
	status_t error = packageReader.Init(fd, false);
	if (error != B_OK)
		RETURN_ERROR(error);

	// parse content
	LoaderContentHandler handler(this);
	error = handler.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	error = packageReader.ParseContent(&handler);
	if (error != B_OK)
		RETURN_ERROR(error);

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
	fFD = openat(fVolume->PackagesDirectoryFD(), fFileName, O_RDONLY);
	if (fFD < 0) {
		ERROR("Failed to open package file \"%s\"\n", fFileName.Data());
		return errno;
	}

	// stat it to verify that it's still the same file
	struct stat st;
	if (fstat(fFD, &st) < 0) {
		ERROR("Failed to stat package file \"%s\"\n", fFileName.Data());
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
	}
}
