/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageLinkSymlink.h"

#include <algorithm>

#include <NodeMonitor.h>

#include "AutoPackageAttributeDirectoryCookie.h"
#include "DebugSupport.h"
#include "NodeListener.h"
#include "PackageLinksListener.h"
#include "Utils.h"
#include "Volume.h"


static const char* const kUnknownLinkTarget = "?";

static const char* const kLinkPaths[PackageLinkSymlink::TYPE_ENUM_COUNT]
		[PACKAGE_FS_MOUNT_TYPE_ENUM_COUNT] = {
	{
		"../..",
		"../../../home/config",
		kUnknownLinkTarget
	},
	{
		"../../../system/settings",
		"../../../home/config/settings/global",
		kUnknownLinkTarget
	}
};


static const char*
link_path_for_mount_type(MountType mountType, PackageLinkSymlink::Type linkType)
{
	if (mountType < 0 || mountType >= PACKAGE_FS_MOUNT_TYPE_ENUM_COUNT
		|| linkType < 0 || linkType >= PackageLinkSymlink::TYPE_ENUM_COUNT) {
		return kUnknownLinkTarget;
	}

	return kLinkPaths[linkType][mountType];
}


// #pragma mark - OldAttributes


struct PackageLinkSymlink::OldAttributes : OldNodeAttributes {
	OldAttributes(const timespec& modifiedTime, off_t fileSize)
		:
		fModifiedTime(modifiedTime),
		fFileSize(fileSize)
	{
	}

	virtual timespec ModifiedTime() const
	{
		return fModifiedTime;
	}

	virtual off_t FileSize() const
	{
		return fFileSize;
	}

private:
	timespec	fModifiedTime;
	off_t		fFileSize;
};


// #pragma mark - PackageLinkSymlink


PackageLinkSymlink::PackageLinkSymlink(Package* package, Type type)
	:
	Node(0),
	fPackage(),
	fLinkPath(kUnknownLinkTarget),
	fType(type)
{
	Update(package, NULL);
}


PackageLinkSymlink::~PackageLinkSymlink()
{
}


void
PackageLinkSymlink::Update(Package* package, PackageLinksListener* listener)
{
	OldAttributes oldAttributes(fModifiedTime, FileSize());

	fPackage = package;

	if (package != NULL) {
		fLinkPath = package->InstallPath();
		if (fLinkPath[0] != '\0') {
			if (fType == TYPE_SETTINGS)
				fLinkPath = ".self/settings";
		} else {
			fLinkPath = link_path_for_mount_type(
				package->Volume()->MountType(), fType);
		}
	} else
		fLinkPath = kUnknownLinkTarget;

	get_real_time(fModifiedTime);

	if (listener != NULL) {
		listener->PackageLinkNodeChanged(this,
			B_STAT_SIZE | B_STAT_MODIFICATION_TIME, oldAttributes);
	}
}


mode_t
PackageLinkSymlink::Mode() const
{
	return S_IFLNK | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
}


timespec
PackageLinkSymlink::ModifiedTime() const
{
	return fModifiedTime;
}


off_t
PackageLinkSymlink::FileSize() const
{
	return strlen(fLinkPath);
}


status_t
PackageLinkSymlink::Read(off_t offset, void* buffer, size_t* bufferSize)
{
	return B_BAD_VALUE;
}


status_t
PackageLinkSymlink::Read(io_request* request)
{
	return B_BAD_VALUE;
}


status_t
PackageLinkSymlink::ReadSymlink(void* buffer, size_t* bufferSize)
{
	size_t linkLength = strnlen(fLinkPath, B_PATH_NAME_LENGTH);

	size_t bytesToCopy = std::min(linkLength, *bufferSize);

	*bufferSize = linkLength;

	memcpy(buffer, fLinkPath, bytesToCopy);
	return B_OK;
}


status_t
PackageLinkSymlink::OpenAttributeDirectory(AttributeDirectoryCookie*& _cookie)
{
	AutoPackageAttributeDirectoryCookie* cookie = new(std::nothrow)
		AutoPackageAttributeDirectoryCookie();
	if (cookie == NULL)
		return B_NO_MEMORY;

	_cookie = cookie;
	return B_OK;
}


status_t
PackageLinkSymlink::OpenAttribute(const StringKey& name, int openMode,
	AttributeCookie*& _cookie)
{
	if (fPackage.Get() == NULL)
		return B_ENTRY_NOT_FOUND;

	return AutoPackageAttributes::OpenCookie(fPackage, name, openMode, _cookie);
}
