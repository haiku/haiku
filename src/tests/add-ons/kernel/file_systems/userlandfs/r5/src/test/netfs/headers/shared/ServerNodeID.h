// ServerNodeID.h

#ifndef NET_FS_SERVER_NODE_ID_H
#define NET_FS_SERVER_NODE_ID_H

#include <SupportDefs.h>

#include "Request.h"

// NodeID
struct NodeID {
	NodeID()
		: volumeID(-1),
		  nodeID(-1)
	{
	}

	NodeID(dev_t volumeID, ino_t nodeID)
		: volumeID(volumeID),
		  nodeID(nodeID)
	{
	}

	NodeID(const NodeID& other)
		: volumeID(other.volumeID),
		  nodeID(other.nodeID)
	{
	}

	uint32 GetHashCode() const
	{
		uint64 v = (uint64)nodeID;
		return (uint32)(v >> 32) ^ (uint32)v ^ (uint32)volumeID;
	}

	NodeID& operator=(const NodeID& other)
	{
		volumeID = other.volumeID;
		nodeID = other.nodeID;
		return *this;
	}

	bool operator==(const NodeID& other) const
	{
		return (volumeID == other.volumeID && nodeID == other.nodeID);
	}

	bool operator!=(const NodeID& other) const
	{
		return !(*this == other);
	}

	dev_t volumeID;
	ino_t nodeID;
};

// ServerNodeID
struct ServerNodeID : RequestMember, NodeID {
								ServerNodeID();
								ServerNodeID(dev_t volumeID, ino_t nodeID);
	virtual						~ServerNodeID();

	virtual	void				ShowAround(RequestMemberVisitor* visitor);

	inline	ServerNodeID&		operator=(const NodeID& other);
};

// =
inline
ServerNodeID&
ServerNodeID::operator=(const NodeID& other)
{
	NodeID::operator=(other);
	return *this;
}

#endif	// NET_FS_SERVER_NODE_ID_H
