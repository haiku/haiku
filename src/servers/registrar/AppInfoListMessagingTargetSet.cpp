/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <TokenSpace.h>

#include "AppInfoListMessagingTargetSet.h"
#include "RosterAppInfo.h"

// constructor
AppInfoListMessagingTargetSet::AppInfoListMessagingTargetSet(
		AppInfoList &list, bool skipRegistrar)
	: fList(list),
	  fIterator(list.It()),
	  fSkipRegistrar(skipRegistrar)
{
	_SkipFilteredOutInfos();
}

// destructor
AppInfoListMessagingTargetSet::~AppInfoListMessagingTargetSet()
{
}

// HasNext
bool
AppInfoListMessagingTargetSet::HasNext() const
{
	return fIterator.IsValid();
}

// Next
bool
AppInfoListMessagingTargetSet::Next(port_id &port, int32 &token)
{
	if (!fIterator.IsValid())
		return false;

	port = (*fIterator)->port;
	token = B_PREFERRED_TOKEN;

	++fIterator;
	_SkipFilteredOutInfos();

	return true;
}

// Rewind
void
AppInfoListMessagingTargetSet::Rewind()
{
	fIterator = fList.It();
}

// Filter
bool
AppInfoListMessagingTargetSet::Filter(const RosterAppInfo *info)
{
	if (!fSkipRegistrar)
		return true;

	return (!fSkipRegistrar || info->team != be_app->Team());
}

// _SkipFilteredOutInfos
void
AppInfoListMessagingTargetSet::_SkipFilteredOutInfos()
{
	while (fIterator.IsValid() && !Filter(*fIterator))
		++fIterator;
}

