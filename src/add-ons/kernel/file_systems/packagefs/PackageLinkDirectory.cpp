/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageLinkDirectory.h"

#include <algorithm>
#include <new>

#include <AutoDeleter.h>

#include "EmptyAttributeDirectoryCookie.h"
#include "DebugSupport.h"
#include "PackageLinksListener.h"
#include "Utils.h"
#include "Version.h"
#include "Volume.h"


static const char* const kSelfLinkName = ".self";
static const char* const kSystemLinkPath = "../..";
static const char* const kCommonLinkPath = "../../../common";
static const char* const kHomeLinkPath = "../../../home/config";


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


// #pragma mark - SelfLink


class PackageLinkDirectory::SelfLink : public Node {
public:
	SelfLink(Package* package)
		:
		Node(0),
		fPackage(package),
		fLinkPath(link_path_for_mount_type(
			package->Domain()->Volume()->MountType()))
	{
		get_real_time(fModifiedTime);
	}

	virtual ~SelfLink()
	{
	}

	virtual status_t Init(Directory* parent, const char* name)
	{
		return Node::Init(parent, name);
	}

	virtual mode_t Mode() const
	{
		return S_IFLNK | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH
			| S_IXOTH;
	}

	virtual uid_t UserID() const
	{
		return 0;
	}

	virtual gid_t GroupID() const
	{
		return 0;
	}

	virtual timespec ModifiedTime() const
	{
		return fModifiedTime;
	}

	virtual off_t FileSize() const
	{
		return strlen(fLinkPath);
	}

	virtual status_t Read(off_t offset, void* buffer, size_t* bufferSize)
	{
		return B_BAD_VALUE;
	}

	virtual status_t Read(io_request* request)
	{
		return B_BAD_VALUE;
	}

	virtual status_t ReadSymlink(void* buffer, size_t* bufferSize)
	{
		size_t toCopy = std::min(strlen(fLinkPath), *bufferSize);
		memcpy(buffer, fLinkPath, toCopy);
		*bufferSize = toCopy;

		return B_OK;
	}

	virtual status_t OpenAttributeDirectory(AttributeDirectoryCookie*& _cookie)
	{
		AttributeDirectoryCookie* cookie
			= new(std::nothrow) EmptyAttributeDirectoryCookie;
		if (cookie == NULL)
			return B_NO_MEMORY;

		_cookie = cookie;
		return B_OK;
	}

	virtual status_t OpenAttribute(const char* name, int openMode,
		AttributeCookie*& _cookie)
	{
		return B_ENTRY_NOT_FOUND;
	}

private:
	Package*	fPackage;
	timespec	fModifiedTime;
	const char*	fLinkPath;
};


// #pragma mark - PackageLinkDirectory


PackageLinkDirectory::PackageLinkDirectory()
	:
	Directory(0)
		// the ID needs to be assigned later, when added to a volume
{
	get_real_time(fModifiedTime);
}


PackageLinkDirectory::~PackageLinkDirectory()
{
	if (fSelfLink != NULL)
		fSelfLink->ReleaseReference();
}


status_t
PackageLinkDirectory::Init(Directory* parent, Package* package)
{
	// create the self link
	fSelfLink = new(std::nothrow) SelfLink(package);
	if (fSelfLink == NULL)
		return B_NO_MEMORY;

	status_t error = fSelfLink->Init(this, kSelfLinkName);
	if (error != B_OK)
		RETURN_ERROR(error);

	AddChild(fSelfLink);

	// compute the allocation size needed for the versioned name
	size_t nameLength = strlen(package->Name());
	size_t size = nameLength + 1;

	Version* version = package->Version();
	if (version != NULL) {
		size += 1 + version->ToString(NULL, 0);
			// + 1 for the '-'
	}

	// allocate the name and compose it
	char* name = (char*)malloc(size);
	if (name == NULL)
		return B_NO_MEMORY;
	MemoryDeleter nameDeleter(name);

	memcpy(name, package->Name(), nameLength + 1);
	if (version != NULL) {
		name[nameLength] = '-';
		version->ToString(name + nameLength + 1, size - nameLength - 1);
	}

	// init the directory/node
	error = Init(parent, name);
		// TODO: This copies the name unnecessarily!
	if (error != B_OK)
		RETURN_ERROR(error);

	// add the package
	AddPackage(package, NULL);

	return B_OK;
}


status_t
PackageLinkDirectory::Init(Directory* parent, const char* name)
{
	return Directory::Init(parent, name);
}


mode_t
PackageLinkDirectory::Mode() const
{
	return S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
}


uid_t
PackageLinkDirectory::UserID() const
{
	return 0;
}


gid_t
PackageLinkDirectory::GroupID() const
{
	return 0;
}


timespec
PackageLinkDirectory::ModifiedTime() const
{
	return fModifiedTime;
}


off_t
PackageLinkDirectory::FileSize() const
{
	return 0;
}


status_t
PackageLinkDirectory::OpenAttributeDirectory(
	AttributeDirectoryCookie*& _cookie)
{
	AttributeDirectoryCookie* cookie
		= new(std::nothrow) EmptyAttributeDirectoryCookie;
	if (cookie == NULL)
		return B_NO_MEMORY;

	_cookie = cookie;
	return B_OK;
}


status_t
PackageLinkDirectory::OpenAttribute(const char* name, int openMode,
	AttributeCookie*& _cookie)
{
	return B_ENTRY_NOT_FOUND;
}


void
PackageLinkDirectory::AddPackage(Package* package,
	PackageLinksListener* listener)
{
	NodeWriteLocker writeLocker(this);

	if (listener != NULL)
		listener->PackageLinkDirectoryRemoved(this);

	// TODO: Add in priority order!
	fPackages.Add(package);
	package->SetLinkDirectory(this);

	if (listener != NULL)
		listener->PackageLinkDirectoryAdded(this);
	// TODO: The notifications should only happen as necessary!
}


void
PackageLinkDirectory::RemovePackage(Package* package,
	PackageLinksListener* listener)
{
	ASSERT(package->LinkDirectory() == this);

	NodeWriteLocker writeLocker(this);

	if (listener != NULL)
		listener->PackageLinkDirectoryRemoved(this);

	package->SetLinkDirectory(NULL);
	fPackages.Remove(package);
	// TODO: Check whether that was the top priority package!

	if (listener != NULL && !IsEmpty())
		listener->PackageLinkDirectoryAdded(this);
	// TODO: The notifications should only happen as necessary!
}
