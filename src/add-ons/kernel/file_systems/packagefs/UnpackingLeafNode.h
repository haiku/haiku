/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef LEAF_NODE_H
#define LEAF_NODE_H


#include "Node.h"
#include "PackageLeafNode.h"
#include "UnpackingNode.h"


class UnpackingLeafNode : public Node, public UnpackingNode {
public:
								UnpackingLeafNode(ino_t id);
	virtual						~UnpackingLeafNode();

	virtual	status_t			VFSInit(dev_t deviceID);
	virtual	void				VFSUninit();

	virtual	mode_t				Mode() const;
	virtual	uid_t				UserID() const;
	virtual	gid_t				GroupID() const;
	virtual	timespec			ModifiedTime() const;
	virtual	off_t				FileSize() const;

	virtual	Node*				GetNode();

	virtual	status_t			AddPackageNode(PackageNode* packageNode);
	virtual	void				RemovePackageNode(PackageNode* packageNode);

	virtual	PackageNode*		GetPackageNode();
	virtual	bool				IsOnlyPackageNode(PackageNode* node) const;
	virtual	bool				WillBeFirstPackageNode(
									PackageNode* packageNode) const;

	virtual	void				PrepareForRemoval();
	virtual	status_t			CloneTransferPackageNodes(ino_t id,
									UnpackingNode*& _newNode);

	virtual	status_t			Read(off_t offset, void* buffer,
									size_t* bufferSize);
	virtual	status_t			Read(io_request* request);

	virtual	status_t			ReadSymlink(void* buffer,
									size_t* bufferSize);

	virtual	status_t			OpenAttributeDirectory(
									AttributeDirectoryCookie*& _cookie);
	virtual	status_t			OpenAttribute(const char* name, int openMode,
									AttributeCookie*& _cookie);

	virtual	status_t			IndexAttribute(AttributeIndexer* indexer);
	virtual	void*				IndexCookieForAttribute(const char* name) const;

private:
	inline	PackageLeafNode*	_ActivePackageNode() const;

private:
			PackageLeafNodeList	fPackageNodes;
			PackageLeafNode*	fFinalPackageNode;
};


#endif	// LEAF_NODE_H
