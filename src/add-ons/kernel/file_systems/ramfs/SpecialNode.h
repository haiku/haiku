/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef SPECIAL_NODE_H
#define SPECIAL_NODE_H


#include "Node.h"


class SpecialNode : public Node {
public:
	SpecialNode(Volume *volume, mode_t mode);
	virtual ~SpecialNode();

	virtual status_t SetSize(off_t newSize);
	virtual off_t GetSize() const;
};


#endif	// SPECIAL_NODE_H
