/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */

#include "SpecialNode.h"

#include "Volume.h"


SpecialNode::SpecialNode(Volume *volume, mode_t mode)
	:
	Node(volume, NODE_TYPE_SPECIAL)
{
	fMode = mode;
}


SpecialNode::~SpecialNode()
{
}


status_t
SpecialNode::SetSize(off_t newSize)
{
	return B_UNSUPPORTED;
}


off_t
SpecialNode::GetSize() const
{
	return 0;
}
