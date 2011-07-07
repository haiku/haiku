/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageLinkSymlink.h"

#include <algorithm>

#include <NodeMonitor.h>

#include "EmptyAttributeDirectoryCookie.h"
#include "DebugSupport.h"
#include "PackageLinksListener.h"
#include "Utils.h"
#include "Volume.h"


static const char* const kSystemLinkPath = "../..";
static const char* const kCommonLinkPath = "../../../common";
static const char* const kHomeLinkPath = "../../../home/config";

static const char* const kUnknownLinkTarget = "?";


static const char*
link_path_for_mount_type(MountType type)
{
	switch (type) {
		case MOUNT_TYPE_SYSTEM:
			return kSystemLinkPath;
		case MOUNT_TYPE_COMMON:
			return kCommonLinkPath;
		case MOUNT_TYPE_HOME:
			return kHomeLinkPath;
		case MOUNT_TYPE_CUSTOM:
		default:
			return "?";
	}
}


// #pragma mark -


PackageLinkSymlink::PackageLinkSymlink(Package* package)
	:
	Node(0),
	fLinkPath(kUnknownLinkTarget)
{
	Update(package, NULL);
}


PackageLinkSymlink::~PackageLinkSymlink()
{
}


void
PackageLinkSymlink::Update(Package* package, PackageLinksListener* listener)
{
	if (package != NULL) {
		fLinkPath = link_path_for_mount_type(
			package->Domain()->Volume()->MountType());
	} else
		fLinkPath = kUnknownLinkTarget;

	get_real_time(fModifiedTime);

	if (listener != NULL) {
		listener->PackageLinkNodeChanged(this,
			B_STAT_SIZE | B_STAT_MODIFICATION_TIME);
	}
}


mode_t
PackageLinkSymlink::Mode() const
{
	return S_IFLNK | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
}


uid_t
PackageLinkSymlink::UserID() const
{
	return 0;
}


gid_t
PackageLinkSymlink::GroupID() const
{
	return 0;
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
	size_t toCopy = std::min(strlen(fLinkPath), *bufferSize);
	memcpy(buffer, fLinkPath, toCopy);
	*bufferSize = toCopy;

	return B_OK;
}


status_t
PackageLinkSymlink::OpenAttributeDirectory(AttributeDirectoryCookie*& _cookie)
{
	AttributeDirectoryCookie* cookie
		= new(std::nothrow) EmptyAttributeDirectoryCookie;
	if (cookie == NULL)
		return B_NO_MEMORY;

	_cookie = cookie;
	return B_OK;
}


status_t
PackageLinkSymlink::OpenAttribute(const char* name, int openMode,
	AttributeCookie*& _cookie)
{
	return B_ENTRY_NOT_FOUND;
}
