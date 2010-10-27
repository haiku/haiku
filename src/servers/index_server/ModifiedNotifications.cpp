/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "ModifiedNotifications.h"

#include "fs_query.h"

#include <MessengerPrivate.h>
#include <syscalls.h>

#include "query_private.h"


NotifyAllQuery::NotifyAllQuery()
	:
	fQueryFd(-1)
{

}


NotifyAllQuery::~NotifyAllQuery()
{
	StopWatching();
}


status_t
NotifyAllQuery::StartWatching(const BVolume& volume, const char* query,
	const BMessenger& target)
{
	if (fQueryFd >= 0)
		return B_NOT_ALLOWED;

	BMessenger::Private messengerPrivate(const_cast<BMessenger&>(target));
	port_id port = messengerPrivate.Port();
	long token = (messengerPrivate.IsPreferredTarget() ? -1
		: messengerPrivate.Token());

	fQueryFd = _kern_open_query(volume.Device(), query, strlen(query),
		B_LIVE_QUERY | B_ATTR_CHANGE_NOTIFICATION, port, token);
	if (fQueryFd < 0)
		return fQueryFd;
	return B_OK;
}


status_t
NotifyAllQuery::StopWatching()
{
	status_t error = B_OK;
	if (fQueryFd >= 0) {
		error = _kern_close(fQueryFd);
		fQueryFd = -1;
	}
	return error;
}


ModfiedNotifications::~ModfiedNotifications()
{
	StopWatching();
}


status_t
ModfiedNotifications::StartWatching(const BVolume& volume, time_t startTime,
	const BMessenger& target)
{
	BString string = "(last_modified>=";
	string << startTime;
	string << ")";
	return fQuery.StartWatching(volume, string.String(), target);
}


status_t
ModfiedNotifications::StopWatching()
{
	return fQuery.StopWatching();
}
