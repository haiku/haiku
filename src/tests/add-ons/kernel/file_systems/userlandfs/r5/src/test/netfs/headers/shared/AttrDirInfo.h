// AttrDirInfo.h

#ifndef NET_FS_ATTR_DIR_INFO_H
#define NET_FS_ATTR_DIR_INFO_H

#include <fs_attr.h>

#include "RequestMemberArray.h"

// AttributeInfo
struct AttributeInfo : public RequestMember {
	virtual	void				ShowAround(RequestMemberVisitor* visitor);

	StringData			name;
	struct attr_info	info;
	Data				data;
};

// AttrDirInfo
struct AttrDirInfo : public FlattenableRequestMember {
								AttrDirInfo();

	virtual	void				ShowAround(RequestMemberVisitor* visitor);
	virtual	status_t			Flatten(RequestFlattener* flattener);
	virtual	status_t			Unflatten(RequestUnflattener* unflattener);

	RequestMemberArray<AttributeInfo> attributeInfos;
	int64						revision;
	bool						isValid;
};

#endif	// NET_FS_ATTR_DIR_INFO_H
