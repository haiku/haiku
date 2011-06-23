/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Directory.h"

#include "DebugSupport.h"
#include "UnpackingAttributeCookie.h"
#include "UnpackingAttributeDirectoryCookie.h"
#include "Utils.h"


Directory::Directory(ino_t id)
	:
	Node(id)
{
}


Directory::~Directory()
{
	Node* child = fChildTable.Clear(true);
	while (child != NULL) {
		Node* next = child->NameHashTableNext();
		child->ReleaseReference();
		child = next;
	}
}


status_t
Directory::Init(Directory* parent, const char* name)
{
	status_t error = fChildTable.Init();
	if (error != B_OK)
		return error;

	return Node::Init(parent, name);
}


status_t
Directory::VFSInit(dev_t deviceID)
{
	return B_OK;
}


void
Directory::VFSUninit()
{
}


mode_t
Directory::Mode() const
{
	if (PackageDirectory* packageDirectory = fPackageDirectories.Head())
		return packageDirectory->Mode();
	return S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
}


uid_t
Directory::UserID() const
{
	if (PackageDirectory* packageDirectory = fPackageDirectories.Head())
		return packageDirectory->UserID();
	return 0;
}


gid_t
Directory::GroupID() const
{
	if (PackageDirectory* packageDirectory = fPackageDirectories.Head())
		return packageDirectory->GroupID();
	return 0;
}


timespec
Directory::ModifiedTime() const
{
	if (PackageDirectory* packageDirectory = fPackageDirectories.Head())
		return packageDirectory->ModifiedTime();

	timespec time = { 0, 0 };
	return time;
}


off_t
Directory::FileSize() const
{
	return 0;
}


Node*
Directory::GetNode()
{
	return this;
}


status_t
Directory::AddPackageNode(PackageNode* packageNode)
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
Directory::RemovePackageNode(PackageNode* packageNode)
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
Directory::GetPackageNode()
{
	return fPackageDirectories.Head();
}


status_t
Directory::Read(off_t offset, void* buffer, size_t* bufferSize)
{
	return B_IS_A_DIRECTORY;
}


status_t
Directory::Read(io_request* request)
{
	return B_IS_A_DIRECTORY;
}


status_t
Directory::ReadSymlink(void* buffer, size_t* bufferSize)
{
	return B_IS_A_DIRECTORY;
}


status_t
Directory::OpenAttributeDirectory(AttributeDirectoryCookie*& _cookie)
{
	return UnpackingAttributeDirectoryCookie::Open(fPackageDirectories.Head(),
		_cookie);
}


status_t
Directory::OpenAttribute(const char* name, int openMode,
	AttributeCookie*& _cookie)
{
	return UnpackingAttributeCookie::Open(fPackageDirectories.Head(), name,
		openMode, _cookie);
}


void
Directory::AddChild(Node* node)
{
	fChildTable.Insert(node);
	fChildList.Add(node);
	node->AcquireReference();
}


void
Directory::RemoveChild(Node* node)
{
	Node* nextNode = fChildList.GetNext(node);

	fChildTable.Remove(node);
	fChildList.Remove(node);
	node->ReleaseReference();

	// adjust directory iterators pointing to the removed child
	for (DirectoryIteratorList::Iterator it = fIterators.GetIterator();
			DirectoryIterator* iterator = it.Next();) {
		if (iterator->node == node)
			iterator->node = nextNode;
	}
}


Node*
Directory::FindChild(const char* name)
{
	return fChildTable.Lookup(name);
}


void
Directory::AddDirectoryIterator(DirectoryIterator* iterator)
{
	fIterators.Add(iterator);
}


void
Directory::RemoveDirectoryIterator(DirectoryIterator* iterator)
{
	fIterators.Remove(iterator);
}


RootDirectory::RootDirectory(ino_t id, const timespec& modifiedTime)
	:
	Directory(id),
	fModifiedTime(modifiedTime)
{
}


timespec
RootDirectory::ModifiedTime() const
{
	return fModifiedTime;
}
