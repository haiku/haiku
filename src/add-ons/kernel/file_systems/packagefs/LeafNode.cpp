/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "LeafNode.h"


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

	fPackageNodes.Add(dynamic_cast<PackageLeafNode*>(packageNode));

	return B_OK;
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


const char*
LeafNode::SymlinkPath() const
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->SymlinkPath();
	return NULL;
}
