// EntryInfo.h

#ifndef NET_FS_ENTRY_INFO_H
#define NET_FS_ENTRY_INFO_H

#include "NodeInfo.h"
#include "Request.h"

struct EntryInfo : public RequestMember {
	virtual	void				ShowAround(RequestMemberVisitor* visitor);

	NodeID		directoryID;
	StringData	name;
	NodeInfo	nodeInfo;
};

#endif	// NET_FS_ENTRY_INFO_H
