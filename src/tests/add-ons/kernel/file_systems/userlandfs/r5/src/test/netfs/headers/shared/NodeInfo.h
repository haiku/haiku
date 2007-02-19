// NodeInfo.h

#ifndef NET_FS_NODE_INFO_H
#define NET_FS_NODE_INFO_H

#include <sys/stat.h>

#include "Request.h"
#include "ServerNodeID.h"

struct NodeInfo : public RequestMember {
	virtual	void				ShowAround(RequestMemberVisitor* visitor);

	inline	NodeID				GetID() const;

	struct stat	st;
	int64		revision;
};

// GetID
inline
NodeID
NodeInfo::GetID() const
{
	return NodeID(st.st_dev, st.st_ino);
}

#endif	// NET_FS_NODE_INFO_H
