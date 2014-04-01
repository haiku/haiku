/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NODE_REF_H
#define NODE_REF_H


#include <Node.h>


inline
node_ref::node_ref()
	:
	device(-1),
	node(-1)
{
}


inline
node_ref::node_ref(dev_t device, ino_t node)
	:
	device(device),
	node(node)
{
}


inline
node_ref::node_ref(const node_ref& other)
	:
	device(other.device),
	node(other.node)
{
}


inline bool
node_ref::operator==(const node_ref& other) const
{
	return device == other.device && node == other.node;
}


inline bool
node_ref::operator!=(const node_ref& other) const
{
	return !(*this == other);
}


inline bool
node_ref::operator<(const node_ref& other) const
{
	if (device != other.device)
		return device < other.device;
	return node < other.node;
}


inline node_ref&
node_ref::operator=(const node_ref& other)
{
	device = other.device;
	node = other.node;
	return *this;
}


#endif	// NODE_REF_H
