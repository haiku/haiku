/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageLinksDirectory.h"

#include <AutoDeleter.h>

#include "EmptyAttributeDirectoryCookie.h"
#include "DebugSupport.h"
#include "Utils.h"


PackageLinksDirectory::PackageLinksDirectory()
	:
	Directory(0)
		// the ID needs to be assigned later, when added to a volume
{
	get_real_time(fModifiedTime);
}


PackageLinksDirectory::~PackageLinksDirectory()
{
}


status_t
PackageLinksDirectory::Init(Directory* parent, const char* name)
{
	status_t error = fPackageFamilies.Init();
	if (error != B_OK)
		RETURN_ERROR(error);

	return Directory::Init(parent, name);
}


mode_t
PackageLinksDirectory::Mode() const
{
	return S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
}


uid_t
PackageLinksDirectory::UserID() const
{
	return 0;
}


gid_t
PackageLinksDirectory::GroupID() const
{
	return 0;
}


timespec
PackageLinksDirectory::ModifiedTime() const
{
	return fModifiedTime;
}


off_t
PackageLinksDirectory::FileSize() const
{
	return 0;
}


status_t
PackageLinksDirectory::OpenAttributeDirectory(
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
PackageLinksDirectory::OpenAttribute(const char* name, int openMode,
	AttributeCookie*& _cookie)
{
	return B_ENTRY_NOT_FOUND;
}


status_t
PackageLinksDirectory::AddPackage(Package* package)
{
	// Create a package family -- there might already be one, but since that's
	// unlikely, we don't bother to check and recheck later.
	PackageFamily* packageFamily = new(std::nothrow) PackageFamily;
	if (packageFamily == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<PackageFamily> packageFamilyDeleter(packageFamily);

	status_t error = packageFamily->Init(package);
	if (error != B_OK)
		RETURN_ERROR(error);

	// add the family
	NodeWriteLocker writeLocker(this);
	if (PackageFamily* otherPackageFamily
			= fPackageFamilies.Lookup(packageFamily->Name())) {
		packageFamily->RemovePackage(package);
		packageFamily = otherPackageFamily;
		packageFamily->AddPackage(package);
	} else
		fPackageFamilies.Insert(packageFamilyDeleter.Detach());

// TODO:...

	return B_OK;
}


void
PackageLinksDirectory::RemovePackage(Package* package)
{
	NodeWriteLocker writeLocker(this);

	PackageFamily* packageFamily = package->Family();
	if (packageFamily == NULL)
		return;

	packageFamily->RemovePackage(package);

	if (packageFamily->IsEmpty()) {
		fPackageFamilies.Remove(packageFamily);
		delete packageFamily;
	}

// TODO:...
}

