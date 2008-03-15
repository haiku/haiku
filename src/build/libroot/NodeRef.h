/*
 * Copyright 2005-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NODE_REF_H
#define NODE_REF_H

#include <sys/stat.h>

namespace BPrivate {

struct NodeRef {
	dev_t	device;
	ino_t	node;

	NodeRef(dev_t device = 0, ino_t node = 0)
		: device(device),
		  node(node)
	{
	}

	NodeRef(const struct stat &st)
		: device(st.st_dev),
		  node(st.st_ino)
	{
	}
	
	NodeRef(const NodeRef &other)
	{
		device = other.device;
		node = other.node;
	}

	NodeRef &operator=(const NodeRef &other)
	{
		device = other.device;
		node = other.node;
		return *this;
	}

	bool operator==(const NodeRef &other) const
	{
		return (device == other.device && node == other.node);
	}

	bool operator!=(const NodeRef &other) const
	{
		return !(*this == other);
	}

	bool operator<(const NodeRef &other) const
	{
		return (device < other.device
			|| (device == other.device && node < other.node));
	}

};

} // namespace BPrivate

#endif	// NODE_REF_H
