/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 *
 * Implements the MessagingTargetSet interface for AppInfoLists, so that
 * no other representation (array/list) is needed to feed them into the
 * MessageDeliverer.
 */
#ifndef APP_INFO_LIST_MESSAGING_TARGET_SET_H
#define APP_INFO_LIST_MESSAGING_TARGET_SET_H

#include "AppInfoList.h"
#include "MessageDeliverer.h"

class RosterAppInfo;

class AppInfoListMessagingTargetSet : public MessagingTargetSet {
public:
	AppInfoListMessagingTargetSet(AppInfoList &list,
		bool skipRegistrar = true);
	virtual ~AppInfoListMessagingTargetSet();

	virtual bool HasNext() const;
	virtual bool Next(port_id &port, int32 &token);
	virtual void Rewind();

	virtual bool Filter(const RosterAppInfo *info);

private:
	void _SkipFilteredOutInfos();

	AppInfoList				&fList;
	AppInfoList::Iterator	fIterator;
	bool					fSkipRegistrar;
};

#endif	// APP_INFO_LIST_MESSAGING_TARGET_SET_H
