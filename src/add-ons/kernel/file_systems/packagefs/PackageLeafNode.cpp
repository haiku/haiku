/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageLeafNode.h"


PackageLeafNode::PackageLeafNode(mode_t mode)
	:
	PackageNode(mode)
{
}


PackageLeafNode::~PackageLeafNode()
{
}


const char*
PackageLeafNode::SymlinkPath() const
{
	return NULL;
}
