/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
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

	virtual	status_t			AddPackageNode(PackageNode* packageNode) = 0;
	virtual	void				RemovePackageNode(PackageNode* packageNode) = 0;

	virtual	PackageNode*		GetPackageNode() = 0;
	virtual	bool				IsOnlyPackageNode(PackageNode* node) const = 0;
};


#endif	// UNPACKING_NODE_H
