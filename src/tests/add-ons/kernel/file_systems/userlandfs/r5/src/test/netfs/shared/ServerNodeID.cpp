// ServerNodeID.cpp

#include "ServerNodeID.h"

// constructor
ServerNodeID::ServerNodeID()
	: RequestMember(),
	  NodeID()
{
}

// constructor
ServerNodeID::ServerNodeID(dev_t volumeID, ino_t nodeID)
	: RequestMember(),
	  NodeID(volumeID, nodeID)
{
}

// destructor
ServerNodeID::~ServerNodeID()
{
}

// ShowAround
void
ServerNodeID::ShowAround(RequestMemberVisitor* visitor)
{
	visitor->Visit(this, volumeID);
	visitor->Visit(this, nodeID);
}

