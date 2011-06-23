/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "LeafNode.h"

#include <string.h>

#include <algorithm>

#include "Utils.h"


LeafNode::LeafNode(ino_t id)
	:
	Node(id)
{
}


LeafNode::~LeafNode()
{
}


status_t
LeafNode::Init(Directory* parent, const char* name)
{
	return Node::Init(parent, name);
}


status_t
LeafNode::VFSInit(dev_t deviceID)
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->VFSInit(deviceID, fID);
	return B_OK;
}


void
LeafNode::VFSUninit()
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		packageNode->VFSUninit();
}


mode_t
LeafNode::Mode() const
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->Mode();
	return S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;
}


uid_t
LeafNode::UserID() const
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->UserID();
	return 0;
}


gid_t
LeafNode::GroupID() const
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->GroupID();
	return 0;
}


timespec
LeafNode::ModifiedTime() const
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->ModifiedTime();

	timespec time = { 0, 0 };
	return time;
}


off_t
LeafNode::FileSize() const
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->FileSize();
	return 0;
}


status_t
LeafNode::AddPackageNode(PackageNode* packageNode)
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
LeafNode::RemovePackageNode(PackageNode* packageNode)
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
LeafNode::GetPackageNode()
{
	return fPackageNodes.Head();
}


status_t
LeafNode::Read(off_t offset, void* buffer, size_t* bufferSize)
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->Read(offset, buffer, bufferSize);
	return B_ERROR;
}


status_t
LeafNode::Read(io_request* request)
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->Read(request);
	return EBADF;
}


status_t
LeafNode::ReadSymlink(void* buffer, size_t* bufferSize)
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


const char*
LeafNode::SymlinkPath() const
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->SymlinkPath();
	return NULL;
}
