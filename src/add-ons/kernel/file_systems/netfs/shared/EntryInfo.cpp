// EntryInfo.cpp

#include "EntryInfo.h"

// ShowAround
void
EntryInfo::ShowAround(RequestMemberVisitor* visitor)
{
	visitor->Visit(this, directoryID.volumeID);
	visitor->Visit(this, directoryID.nodeID);
	visitor->Visit(this, name);
	visitor->Visit(this, nodeInfo);
}

