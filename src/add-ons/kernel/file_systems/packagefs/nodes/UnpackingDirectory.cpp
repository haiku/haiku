/*
 * Copyright 2009-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "UnpackingDirectory.h"

#include "DebugSupport.h"
#include "EmptyAttributeDirectoryCookie.h"
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


status_t
UnpackingDirectory::VFSInit(dev_t deviceID)
{
	status_t error = NodeInitVFS(deviceID, fID, fPackageDirectories.Head());
	if (error == B_OK)
		Directory::VFSInit(deviceID);

	return error;
}


void
UnpackingDirectory::VFSUninit()
{
	NodeUninitVFS(fPackageDirectories.Head(), fFlags);
	Directory::VFSUninit();
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
UnpackingDirectory::AddPackageNode(PackageNode* packageNode, dev_t deviceID)
{
	if (!S_ISDIR(packageNode->Mode()))
		return B_NOT_A_DIRECTORY;

	PackageDirectory* packageDirectory
		= dynamic_cast<PackageDirectory*>(packageNode);

	PackageDirectory* other = fPackageDirectories.Head();
	bool overridesHead = other == NULL
		|| packageDirectory->HasPrecedenceOver(other);

	if (overridesHead) {
		fPackageDirectories.InsertBefore(other, packageDirectory);
		NodeReinitVFS(deviceID, fID, packageDirectory, other, fFlags);
	} else
		fPackageDirectories.Add(packageDirectory);

	return B_OK;
}


void
UnpackingDirectory::RemovePackageNode(PackageNode* packageNode, dev_t deviceID)
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
			if (otherNode->HasPrecedenceOver(newestNode))
				newestNode = otherNode;
		}

		fPackageDirectories.Remove(newestNode);
		fPackageDirectories.InsertBefore(fPackageDirectories.Head(), newestNode);
		NodeReinitVFS(deviceID, fID, newestNode, packageNode, fFlags);
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
	return other == NULL || packageDirectory->HasPrecedenceOver(other);
}


void
UnpackingDirectory::PrepareForRemoval()
{
	fPackageDirectories.MakeEmpty();
}


status_t
UnpackingDirectory::OpenAttributeDirectory(AttributeDirectoryCookie*& _cookie)
{
	if (HasVFSInitError())
		return B_ERROR;

	return UnpackingAttributeDirectoryCookie::Open(fPackageDirectories.Head(),
		_cookie);
}


status_t
UnpackingDirectory::OpenAttribute(const StringKey& name, int openMode,
	AttributeCookie*& _cookie)
{
	if (HasVFSInitError())
		return B_ERROR;

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
UnpackingDirectory::IndexCookieForAttribute(const StringKey& name) const
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


status_t
RootDirectory::OpenAttributeDirectory(AttributeDirectoryCookie*& _cookie)
{
	if (HasVFSInitError())
		return B_ERROR;

	_cookie = new(std::nothrow) EmptyAttributeDirectoryCookie;
	if (_cookie == nullptr)
		return B_NO_MEMORY;
	return B_OK;
}


timespec
RootDirectory::ModifiedTime() const
{
	return fModifiedTime;
}
