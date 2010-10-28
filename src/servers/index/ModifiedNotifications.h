/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef MODIFIED_NOTIFICATIONS_H
#define MODIFIED_NOTIFICATIONS_H


#include <String.h>
#include <Volume.h>
#include <Messenger.h>


class NotifyAllQuery {
public:
								NotifyAllQuery();
								~NotifyAllQuery();

			status_t			StartWatching(const BVolume& volume,
									const char* query,
									const BMessenger& target);
			status_t			StopWatching();
private:
			int					fQueryFd;
};


class ModfiedNotifications {
public:
								~ModfiedNotifications();

			status_t			StartWatching(const BVolume& volume,
									time_t startTime, const BMessenger& target);
			status_t			StopWatching();

private:
			NotifyAllQuery		fQuery;
};


#endif // MODIFIED_NOTIFICATIONS_H
