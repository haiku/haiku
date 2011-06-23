/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef LEAF_NODE_H
#define LEAF_NODE_H


#include "Node.h"
#include "PackageLeafNode.h"


class LeafNode : public Node {
public:
								LeafNode(ino_t id);
	virtual						~LeafNode();

	virtual	status_t			Init(Directory* parent, const char* name);

	virtual	status_t			VFSInit(dev_t deviceID);
	virtual	void				VFSUninit();

	virtual	mode_t				Mode() const;
	virtual	uid_t				UserID() const;
	virtual	gid_t				GroupID() const;
	virtual	timespec			ModifiedTime() const;
	virtual	off_t				FileSize() const;

	virtual	status_t			AddPackageNode(PackageNode* packageNode);
	virtual	void				RemovePackageNode(PackageNode* packageNode);

	virtual	PackageNode*		GetPackageNode();

	virtual	status_t			Read(off_t offset, void* buffer,
									size_t* bufferSize);
	virtual	status_t			Read(io_request* request);

	virtual	status_t			ReadSymlink(void* buffer,
									size_t* bufferSize);

	virtual	status_t			OpenAttribute(const char* name, int openMode,
									AttributeCookie*& _cookie);

			const char*			SymlinkPath() const;

private:
			PackageLeafNodeList	fPackageNodes;
};


#endif	// LEAF_NODE_H
