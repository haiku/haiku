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


status_t
LeafNode::AddPackageNode(PackageNode* packageNode)
{
	if (S_ISDIR(packageNode->Mode()))
		return B_BAD_VALUE;

	fPackageNodes.Add(dynamic_cast<PackageLeafNode*>(packageNode));

	return B_OK;
}


const char*
LeafNode::SymlinkPath() const
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode->SymlinkPath();
	return NULL;
}
