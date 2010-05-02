/*
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NOTIFICATION_MANAGER_H
#define NOTIFICATION_MANAGER_H


#include <Locker.h>
#include <MediaNode.h>
#include <Messenger.h>

#include "Queue.h"
#include "TList.h"


struct Notification {
	BMessenger	messenger;
	media_node	node;
	int32		what;
	team_id		team;
};

class NotificationManager {
public:
								NotificationManager();
								~NotificationManager();

			void				Dump();

			void				EnqueueMessage(BMessage* message);

			void				CleanupTeam(team_id team);

private:
			void				RequestNotifications(BMessage* message);
			void				CancelNotifications(BMessage* message);
			void				SendNotifications(BMessage* message);

			void				WorkerThread();
	static	int32				worker_thread(void* arg);

private:
			Queue				fNotificationQueue;
			thread_id			fNotificationThreadId;
			BLocker				fLocker;
			List<Notification>	fNotificationList;
};

#endif	// NOTIFICATION_MANAGER_H
