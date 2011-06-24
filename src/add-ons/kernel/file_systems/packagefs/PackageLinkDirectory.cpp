/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageLinkDirectory.h"

#include <AutoDeleter.h>

#include "EmptyAttributeDirectoryCookie.h"
#include "DebugSupport.h"
#include "Utils.h"
#include "Version.h"


PackageLinkDirectory::PackageLinkDirectory()
	:
	Directory(0)
		// the ID needs to be assigned later, when added to a volume
{
	get_real_time(fModifiedTime);
}


PackageLinkDirectory::~PackageLinkDirectory()
{
}


status_t
PackageLinkDirectory::Init(Directory* parent, Package* package)
{
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
	status_t error = Init(parent, name);
		// TODO: This copies the name unnecessarily!
	if (error != B_OK)
		RETURN_ERROR(error);

	// add the package
	AddPackage(package);

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
PackageLinkDirectory::AddPackage(Package* package)
{
	// TODO: Add in priority order!
	fPackages.Add(package);
	package->SetLinkDirectory(this);
}


void
PackageLinkDirectory::RemovePackage(Package* package)
{

	ASSERT(package->LinkDirectory() == this);

	package->SetLinkDirectory(NULL);
	fPackages.Remove(package);
	// TODO: Check whether that was the top priority package!
}
