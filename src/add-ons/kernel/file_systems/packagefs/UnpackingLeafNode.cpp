/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "UnpackingLeafNode.h"

#include <string.h>

#include <algorithm>

#include "UnpackingAttributeCookie.h"
#include "UnpackingAttributeDirectoryCookie.h"
#include "Utils.h"


UnpackingLeafNode::UnpackingLeafNode(ino_t id)
	:
	Node(id)
{
}


UnpackingLeafNode::~UnpackingLeafNode()
{
}


status_t
UnpackingLeafNode::VFSInit(dev_t deviceID)
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->VFSInit(deviceID, fID);
	return B_OK;
}


void
UnpackingLeafNode::VFSUninit()
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		packageNode->VFSUninit();
}


mode_t
UnpackingLeafNode::Mode() const
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->Mode();
	return S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;
}


uid_t
UnpackingLeafNode::UserID() const
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->UserID();
	return 0;
}


gid_t
UnpackingLeafNode::GroupID() const
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->GroupID();
	return 0;
}


timespec
UnpackingLeafNode::ModifiedTime() const
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->ModifiedTime();

	timespec time = { 0, 0 };
	return time;
}


off_t
UnpackingLeafNode::FileSize() const
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->FileSize();
	return 0;
}


Node*
UnpackingLeafNode::GetNode()
{
	return this;
}


status_t
UnpackingLeafNode::AddPackageNode(PackageNode* packageNode)
{
	if (S_ISDIR(packageNode->Mode()))
		return B_BAD_VALUE;

	PackageLeafNode* packageLeafNode
		= dynamic_cast<PackageLeafNode*>(packageNode);

	PackageLeafNode* headNode = fPackageNodes.Head();
	bool isNewest = headNode == NULL
		|| packageLeafNode->ModifiedTime() > headNode->ModifiedTime();

	if (isNewest) {
		fPackageNodes.Add(packageLeafNode);
	} else {
		// add after the head
		fPackageNodes.RemoveHead();
		fPackageNodes.Add(packageLeafNode);
		fPackageNodes.Add(headNode);
	}

	return B_OK;
}


void
UnpackingLeafNode::RemovePackageNode(PackageNode* packageNode)
{
	bool isNewest = packageNode == fPackageNodes.Head();
	fPackageNodes.Remove(dynamic_cast<PackageLeafNode*>(packageNode));

	// when removing the newest node, we need to find the next node (the list
	// is not sorted)
	PackageLeafNode* newestNode = fPackageNodes.Head();
	if (isNewest && newestNode != NULL) {
		PackageLeafNodeList::Iterator it = fPackageNodes.GetIterator();
		it.Next();
			// skip the first one
		while (PackageLeafNode* otherNode = it.Next()) {
			if (otherNode->ModifiedTime() > newestNode->ModifiedTime())
				newestNode = otherNode;
		}

		// re-add the newest node to the head
		fPackageNodes.Remove(newestNode);
		fPackageNodes.Add(newestNode);
	}
}


PackageNode*
UnpackingLeafNode::GetPackageNode()
{
	return fPackageNodes.Head();
}


status_t
UnpackingLeafNode::Read(off_t offset, void* buffer, size_t* bufferSize)
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->Read(offset, buffer, bufferSize);
	return B_ERROR;
}


status_t
UnpackingLeafNode::Read(io_request* request)
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->Read(request);
	return EBADF;
}


status_t
UnpackingLeafNode::ReadSymlink(void* buffer, size_t* bufferSize)
{
	PackageLeafNode* packageNode = fPackageNodes.Head();
	if (packageNode == NULL)
		return B_BAD_VALUE;

	const char* linkPath = packageNode->SymlinkPath();
	if (linkPath == NULL) {
		*bufferSize = 0;
		return B_OK;
	}

	size_t toCopy = std::min(strlen(linkPath), *bufferSize);
	memcpy(buffer, linkPath, toCopy);
	*bufferSize = toCopy;

	return B_OK;
}


status_t
UnpackingLeafNode::OpenAttributeDirectory(AttributeDirectoryCookie*& _cookie)
{
	return UnpackingAttributeDirectoryCookie::Open(fPackageNodes.Head(),
		_cookie);
}


status_t
UnpackingLeafNode::OpenAttribute(const char* name, int openMode,
	AttributeCookie*& _cookie)
{
	return UnpackingAttributeCookie::Open(fPackageNodes.Head(), name, openMode,
		_cookie);
}


status_t
UnpackingLeafNode::IndexAttribute(AttributeIndexer* indexer)
{
	return UnpackingAttributeCookie::IndexAttribute(fPackageNodes.Head(),
		indexer);
}


void*
UnpackingLeafNode::IndexCookieForAttribute(const char* name) const
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->IndexCookieForAttribute(name);
	return NULL;
}
