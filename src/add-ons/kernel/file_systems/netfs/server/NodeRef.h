// NodeRef.h

#ifndef NET_FS_NODE_REF_H
#define NET_FS_NODE_REF_H

#include <Node.h>

struct NodeRef : node_ref {
	NodeRef() {}

	NodeRef(const node_ref& ref)
		: node_ref(ref)
	{
	}

	NodeRef(dev_t volumeID, ino_t nodeID)
	{
		device = volumeID;
		node = nodeID;
	}

	NodeRef(const NodeRef& other)
		: node_ref(other)
	{
	}

	uint32 GetHashCode() const
	{
		uint64 v = (uint64)node;
		return (uint32)(v >> 32) ^ (uint32)v ^ (uint32)device;
	}

	NodeRef& operator=(const node_ref& other)
	{
		node_ref::operator=(other);
		return *this;
	}

	bool operator==(const node_ref& other) const
	{
		return node_ref::operator==(other);
	}

	bool operator!=(const NodeRef& other) const
	{
		return !(*this == other);
	}
};

#endif	// NET_FS_NODE_REF_H
