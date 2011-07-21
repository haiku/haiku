/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "UnpackingDirectory.h"

#include "DebugSupport.h"
#include "UnpackingAttributeCookie.h"
#include "UnpackingAttributeDirectoryCookie.h"
#include "Utils.h"


// #pragma mark - UnpackingDirectory


UnpackingDirectory::UnpackingDirectory(ino_t id)
	:
	Directory(id)
{
}


UnpackingDirectory::~UnpackingDirectory()
{
}


mode_t
UnpackingDirectory::Mode() const
{
	if (PackageDirectory* packageDirectory = fPackageDirectories.Head())
		return packageDirectory->Mode();
	return S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
}


uid_t
UnpackingDirectory::UserID() const
{
	if (PackageDirectory* packageDirectory = fPackageDirectories.Head())
		return packageDirectory->UserID();
	return 0;
}


gid_t
UnpackingDirectory::GroupID() const
{
	if (PackageDirectory* packageDirectory = fPackageDirectories.Head())
		return packageDirectory->GroupID();
	return 0;
}


timespec
UnpackingDirectory::ModifiedTime() const
{
	if (PackageDirectory* packageDirectory = fPackageDirectories.Head())
		return packageDirectory->ModifiedTime();

	timespec time = { 0, 0 };
	return time;
}


off_t
UnpackingDirectory::FileSize() const
{
	return 0;
}


Node*
UnpackingDirectory::GetNode()
{
	return this;
}


status_t
UnpackingDirectory::AddPackageNode(PackageNode* packageNode)
{
	if (!S_ISDIR(packageNode->Mode()))
		return B_BAD_VALUE;

	PackageDirectory* packageDirectory
		= dynamic_cast<PackageDirectory*>(packageNode);

	PackageDirectory* other = fPackageDirectories.Head();
	bool isNewest = other == NULL
		|| packageDirectory->ModifiedTime() > other->ModifiedTime();

	if (isNewest)
		fPackageDirectories.Insert(other, packageDirectory);
	else
		fPackageDirectories.Add(packageDirectory);

	return B_OK;
}


void
UnpackingDirectory::RemovePackageNode(PackageNode* packageNode)
{
	bool isNewest = packageNode == fPackageDirectories.Head();
	fPackageDirectories.Remove(dynamic_cast<PackageDirectory*>(packageNode));

	// when removing the newest node, we need to find the next node (the list
	// is not sorted)
	PackageDirectory* newestNode = fPackageDirectories.Head();
	if (isNewest && newestNode != NULL) {
		PackageDirectoryList::Iterator it = fPackageDirectories.GetIterator();
		it.Next();
			// skip the first one
		while (PackageDirectory* otherNode = it.Next()) {
			if (otherNode->ModifiedTime() > newestNode->ModifiedTime())
				newestNode = otherNode;
		}

		fPackageDirectories.Remove(newestNode);
		fPackageDirectories.Insert(fPackageDirectories.Head(), newestNode);
	}
}


PackageNode*
UnpackingDirectory::GetPackageNode()
{
	return fPackageDirectories.Head();
}


bool
UnpackingDirectory::IsOnlyPackageNode(PackageNode* node) const
{
	return node == fPackageDirectories.Head()
		&& node == fPackageDirectories.Tail();
}


bool
UnpackingDirectory::WillBeFirstPackageNode(PackageNode* packageNode) const
{
	PackageDirectory* packageDirectory
		= dynamic_cast<PackageDirectory*>(packageNode);
	if (packageDirectory == NULL)
		return false;

	PackageDirectory* other = fPackageDirectories.Head();
	return other == NULL
		|| packageDirectory->ModifiedTime() > other->ModifiedTime();
}


void
UnpackingDirectory::PrepareForRemoval()
{
	fPackageDirectories.MakeEmpty();
}


status_t
UnpackingDirectory::OpenAttributeDirectory(AttributeDirectoryCookie*& _cookie)
{
	return UnpackingAttributeDirectoryCookie::Open(fPackageDirectories.Head(),
		_cookie);
}


status_t
UnpackingDirectory::OpenAttribute(const char* name, int openMode,
	AttributeCookie*& _cookie)
{
	return UnpackingAttributeCookie::Open(fPackageDirectories.Head(), name,
		openMode, _cookie);
}


status_t
UnpackingDirectory::IndexAttribute(AttributeIndexer* indexer)
{
	return UnpackingAttributeCookie::IndexAttribute(fPackageDirectories.Head(),
		indexer);
}


void*
UnpackingDirectory::IndexCookieForAttribute(const char* name) const
{
	if (PackageDirectory* packageDirectory = fPackageDirectories.Head())
		return packageDirectory->IndexCookieForAttribute(name);
	return NULL;
}


// #pragma mark - RootDirectory


RootDirectory::RootDirectory(ino_t id, const timespec& modifiedTime)
	:
	UnpackingDirectory(id),
	fModifiedTime(modifiedTime)
{
}


timespec
RootDirectory::ModifiedTime() const
{
	return fModifiedTime;
}
