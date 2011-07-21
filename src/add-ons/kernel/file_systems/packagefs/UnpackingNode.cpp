/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "UnpackingNode.h"


UnpackingNode::~UnpackingNode()
{
}


status_t
UnpackingNode::CloneTransferPackageNodes(ino_t id, UnpackingNode*& _newNode)
{
	return B_BAD_VALUE;
}
