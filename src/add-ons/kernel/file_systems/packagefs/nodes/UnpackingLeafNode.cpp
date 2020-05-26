/*
 * Copyright 2009-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "UnpackingLeafNode.h"

#include <string.h>

#include <algorithm>
#include <new>

#include "UnpackingAttributeCookie.h"
#include "UnpackingAttributeDirectoryCookie.h"
#include "Utils.h"


UnpackingLeafNode::UnpackingLeafNode(ino_t id)
	:
	Node(id),
	fFinalPackageNode(NULL)
{
}


UnpackingLeafNode::~UnpackingLeafNode()
{
	if (fFinalPackageNode != NULL)
		fFinalPackageNode->ReleaseReference();
}


status_t
UnpackingLeafNode::VFSInit(dev_t deviceID)
{
	status_t error = NodeInitVFS(deviceID, fID, _ActivePackageNode());
	if (error == B_OK)
		Node::VFSInit(deviceID);

	return error;
}


void
UnpackingLeafNode::VFSUninit()
{
	NodeUninitVFS(_ActivePackageNode(), fFlags);
	Node::VFSUninit();
}


mode_t
UnpackingLeafNode::Mode() const
{
	if (PackageLeafNode* packageNode = _ActivePackageNode())
		return packageNode->Mode();
	return S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;
}


uid_t
UnpackingLeafNode::UserID() const
{
	if (PackageLeafNode* packageNode = _ActivePackageNode())
		return packageNode->UserID();
	return 0;
}


gid_t
UnpackingLeafNode::GroupID() const
{
	if (PackageLeafNode* packageNode = _ActivePackageNode())
		return packageNode->GroupID();
	return 0;
}


timespec
UnpackingLeafNode::ModifiedTime() const
{
	if (PackageLeafNode* packageNode = _ActivePackageNode())
		return packageNode->ModifiedTime();

	timespec time = { 0, 0 };
	return time;
}


off_t
UnpackingLeafNode::FileSize() const
{
	if (PackageLeafNode* packageNode = _ActivePackageNode()) {
		if (S_ISLNK(packageNode->Mode()))
			return strlen(packageNode->SymlinkPath());
		return packageNode->FileSize();
	}
	return 0;
}


Node*
UnpackingLeafNode::GetNode()
{
	return this;
}


status_t
UnpackingLeafNode::AddPackageNode(PackageNode* packageNode, dev_t deviceID)
{
	ASSERT(fFinalPackageNode == NULL);

	if (S_ISDIR(packageNode->Mode()))
		return B_BAD_VALUE;

	PackageLeafNode* packageLeafNode
		= dynamic_cast<PackageLeafNode*>(packageNode);

	PackageLeafNode* headNode = fPackageNodes.Head();
	bool overridesHead = headNode == NULL
		|| packageLeafNode->HasPrecedenceOver(headNode);

	if (overridesHead) {
		fPackageNodes.Add(packageLeafNode);
		NodeReinitVFS(deviceID, fID, packageLeafNode, headNode, fFlags);
	} else {
		// add after the head
		fPackageNodes.RemoveHead();
		fPackageNodes.Add(packageLeafNode);
		fPackageNodes.Add(headNode);
	}

	return B_OK;
}


void
UnpackingLeafNode::RemovePackageNode(PackageNode* packageNode, dev_t deviceID)
{
	ASSERT(fFinalPackageNode == NULL);

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
			if (otherNode->HasPrecedenceOver(newestNode))
				newestNode = otherNode;
		}

		// re-add the newest node to the head
		fPackageNodes.Remove(newestNode);
		fPackageNodes.Add(newestNode);
		NodeReinitVFS(deviceID, fID, newestNode, packageNode, fFlags);
	}
}


PackageNode*
UnpackingLeafNode::GetPackageNode()
{
	return _ActivePackageNode();
}


bool
UnpackingLeafNode::IsOnlyPackageNode(PackageNode* node) const
{
	ASSERT(fFinalPackageNode == NULL);

	PackageLeafNode* head = fPackageNodes.Head();
	return node == head && fPackageNodes.GetNext(head) == NULL;
}


bool
UnpackingLeafNode::WillBeFirstPackageNode(PackageNode* packageNode) const
{
	PackageLeafNode* packageLeafNode
		= dynamic_cast<PackageLeafNode*>(packageNode);
	if (packageLeafNode == NULL)
		return false;

	PackageLeafNode* headNode = fPackageNodes.Head();
	return headNode == NULL
		|| packageLeafNode->HasPrecedenceOver(headNode);
}

void
UnpackingLeafNode::PrepareForRemoval()
{
	ASSERT(fFinalPackageNode == NULL);

	fFinalPackageNode = fPackageNodes.Head();
	if (fFinalPackageNode != NULL) {
		fFinalPackageNode->AcquireReference();
		fPackageNodes.MakeEmpty();
	}
}


status_t
UnpackingLeafNode::CloneTransferPackageNodes(ino_t id, UnpackingNode*& _newNode)
{
	ASSERT(fFinalPackageNode == NULL);

	UnpackingLeafNode* clone = new(std::nothrow) UnpackingLeafNode(id);
	if (clone == NULL)
		return B_NO_MEMORY;

	status_t error = clone->Init(Parent(), Name());
	if (error != B_OK) {
		delete clone;
		return error;
	}

	// We keep the old head as fFinalPackageNode, which will make us to behave
	// exactly as before with respect to FS operations.
	fFinalPackageNode = fPackageNodes.Head();
	if (fFinalPackageNode != NULL) {
		fFinalPackageNode->AcquireReference();
		clone->fPackageNodes.MoveFrom(&fPackageNodes);
	}

	_newNode = clone;
	return B_OK;
}


status_t
UnpackingLeafNode::Read(off_t offset, void* buffer, size_t* bufferSize)
{
	if (HasVFSInitError())
		return B_ERROR;

	if (PackageLeafNode* packageNode = _ActivePackageNode())
		return packageNode->Read(offset, buffer, bufferSize);
	return B_ERROR;
}


status_t
UnpackingLeafNode::Read(io_request* request)
{
	if (HasVFSInitError())
		return B_ERROR;

	if (PackageLeafNode* packageNode = _ActivePackageNode())
		return packageNode->Read(request);
	return EBADF;
}


status_t
UnpackingLeafNode::ReadSymlink(void* buffer, size_t* bufferSize)
{
	if (HasVFSInitError())
		return B_ERROR;

	PackageLeafNode* packageNode = _ActivePackageNode();
	if (packageNode == NULL)
		return B_BAD_VALUE;

	const String& linkPath = packageNode->SymlinkPath();
	if (linkPath[0] == '\0') {
		*bufferSize = 0;
		return B_OK;
	}

	size_t linkLength = strnlen(linkPath, B_PATH_NAME_LENGTH);

	size_t bytesToCopy = std::min(linkLength, *bufferSize);

	*bufferSize = linkLength;

	memcpy(buffer, linkPath, bytesToCopy);
	return B_OK;
}


status_t
UnpackingLeafNode::OpenAttributeDirectory(AttributeDirectoryCookie*& _cookie)
{
	if (HasVFSInitError())
		return B_ERROR;

	return UnpackingAttributeDirectoryCookie::Open(_ActivePackageNode(),
		_cookie);
}


status_t
UnpackingLeafNode::OpenAttribute(const StringKey& name, int openMode,
	AttributeCookie*& _cookie)
{
	if (HasVFSInitError())
		return B_ERROR;

	return UnpackingAttributeCookie::Open(_ActivePackageNode(), name, openMode,
		_cookie);
}


status_t
UnpackingLeafNode::IndexAttribute(AttributeIndexer* indexer)
{
	return UnpackingAttributeCookie::IndexAttribute(_ActivePackageNode(),
		indexer);
}


void*
UnpackingLeafNode::IndexCookieForAttribute(const StringKey& name) const
{
	if (PackageLeafNode* packageNode = _ActivePackageNode())
		return packageNode->IndexCookieForAttribute(name);
	return NULL;
}


PackageLeafNode*
UnpackingLeafNode::_ActivePackageNode() const
{
	if (PackageLeafNode* packageNode = fPackageNodes.Head())
		return packageNode;
	return fFinalPackageNode;
}
