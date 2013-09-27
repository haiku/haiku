/*
 * Copyright 2011-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNPACKING_NODE_H
#define UNPACKING_NODE_H


#include <SupportDefs.h>


class Node;
class PackageNode;


class UnpackingNode {
public:
	virtual						~UnpackingNode();

	virtual	Node*				GetNode() = 0;

	virtual	status_t			AddPackageNode(PackageNode* packageNode,
									dev_t deviceID) = 0;
	virtual	void				RemovePackageNode(PackageNode* packageNode,
									dev_t deviceID) = 0;

	virtual	PackageNode*		GetPackageNode() = 0;
	virtual	bool				IsOnlyPackageNode(PackageNode* node) const = 0;
	virtual	bool				WillBeFirstPackageNode(
									PackageNode* packageNode) const = 0;

	virtual	void				PrepareForRemoval() = 0;
	virtual	status_t			CloneTransferPackageNodes(ino_t id,
									UnpackingNode*& _newNode);

protected:
			status_t			NodeInitVFS(dev_t deviceID, ino_t nodeID,
									PackageNode* packageNode);
			void				NodeUninitVFS(PackageNode* packageNode,
									uint32& nodeFlags);
			void				NodeReinitVFS(dev_t deviceID, ino_t nodeID,
									PackageNode* packageNode,
						 			PackageNode* previousPackageNode,
									uint32& nodeFlags);
};


#endif	// UNPACKING_NODE_H
